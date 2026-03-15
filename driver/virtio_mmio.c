#include "driver.h"
#include "log.h"
#include "virtio_mmio.h"

struct virtqueue disk_queue __aligned(BLKSZ);
static struct virtio_blk_io disk_io;

__always_inline u32 vio_read(u32 reg)
{
	return *VIRTIO_MMIO_REG(reg);
}

__always_inline void vio_write(u32 reg, u32 val)
{
	*VIRTIO_MMIO_REG(reg) = val;
}

#define virtio_mb() __asm__ volatile("fence ow, ir" ::: "memory")

void blkinit(void)
{
	const u32 magic = vio_read(VIRTIO_MMIO_MAGIC_OFFSET);
	if (magic != VIRTIO_MMIO_MAGIC_VALUE)
		panic("blkinit: bad magic 0x%x", magic);

	const u32 version = vio_read(VIRTIO_MMIO_VERSION_OFFSET);
	if (version != VIRTIO_MMIO_VERSION)
		panic("blkinit: unsupported version %u (want 2)", version);

	const u32 device_id = vio_read(VIRTIO_MMIO_DEVICEID_OFFSET);
	if (device_id != VIRTIO_MMIO_DEVICEID_BLK)
		panic("blkinit: unexpected device id %u", device_id);

	vio_write(VIRTIO_MMIO_STATUS_OFFSET, 0);
	while (vio_read(VIRTIO_MMIO_STATUS_OFFSET) != 0)
		;

	vio_write(VIRTIO_MMIO_STATUS_OFFSET, VIRTIO_STATUS_ACKNOWLEDGE);

	vio_write(VIRTIO_MMIO_STATUS_OFFSET,
		  VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

	vio_write(VIRTIO_MMIO_DEVICE_FEATURES_SEL_OFFSET, 0);
	vio_read(VIRTIO_MMIO_DEVICE_FEATURES_OFFSET);

	vio_write(VIRTIO_MMIO_DEVICE_FEATURES_SEL_OFFSET, 1);
	vio_read(VIRTIO_MMIO_DEVICE_FEATURES_OFFSET);

	vio_write(VIRTIO_MMIO_DRIVER_FEATURES_SEL_OFFSET, 0);
	vio_write(VIRTIO_MMIO_DRIVER_FEATURES_OFFSET, 0);

	vio_write(VIRTIO_MMIO_DRIVER_FEATURES_SEL_OFFSET, 1);
	vio_write(VIRTIO_MMIO_DRIVER_FEATURES_OFFSET, VIRTIO_F_VERSION_1);

	vio_write(VIRTIO_MMIO_STATUS_OFFSET, VIRTIO_STATUS_ACKNOWLEDGE |
						     VIRTIO_STATUS_DRIVER |
						     VIRTIO_STATUS_FEATURES_OK);

	const u32 status_check = vio_read(VIRTIO_MMIO_STATUS_OFFSET);
	if (!(status_check & VIRTIO_STATUS_FEATURES_OK))
		panic("blkinit: device rejected negotiated features");

	vio_write(VIRTIO_MMIO_QUEUE_SEL_OFFSET, 0);

	if (vio_read(VIRTIO_MMIO_QUEUE_READY_OFFSET) != 0)
		panic("blkinit: queue 0 is already active");

	const u32 qmax = vio_read(VIRTIO_MMIO_QUEUE_NUM_MAX_OFFSET);
	if (qmax == 0)
		panic("blkinit: queue 0 not available");
	if (qmax < QSIZE)
		panic("blkinit: QueueNumMax %u < QSIZE %u", qmax, QSIZE);

	vio_write(VIRTIO_MMIO_QUEUE_NUM_OFFSET, QSIZE);

	const auto desc_addr = (uintptr_t)&disk_queue.desc;
	const auto avail_addr = (uintptr_t)&disk_queue.avail;
	const auto used_addr = (uintptr_t)&disk_queue.used;

	vio_write(VIRTIO_MMIO_QUEUE_DESC_LOW_OFFSET, (u32)desc_addr);
	vio_write(VIRTIO_MMIO_QUEUE_DESC_HIGH_OFFSET, (u32)(desc_addr >> 32));
	vio_write(VIRTIO_MMIO_QUEUE_DRIVER_LOW_OFFSET, (u32)avail_addr);
	vio_write(VIRTIO_MMIO_QUEUE_DRIVER_HIGH_OFFSET,
		  (u32)(avail_addr >> 32));
	vio_write(VIRTIO_MMIO_QUEUE_DEVICE_LOW_OFFSET, (u32)used_addr);
	vio_write(VIRTIO_MMIO_QUEUE_DEVICE_HIGH_OFFSET, (u32)(used_addr >> 32));

	vio_write(VIRTIO_MMIO_QUEUE_READY_OFFSET, 1);

	vio_write(VIRTIO_MMIO_STATUS_OFFSET,
		  VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
			  VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK);

	Log("Virtioblk mmio init success");
}

static void virtio_blk_submit(u32 type, u64 sector, char *data)
{
	disk_io.hdr.type = type;
	disk_io.hdr.reserved = 0;
	disk_io.hdr.sector = sector;
	disk_io.status = 0xff;

	disk_queue.desc[0].addr = (uintptr_t)&disk_io.hdr;
	disk_queue.desc[0].len = sizeof(disk_io.hdr);
	disk_queue.desc[0].flags = VIRTQ_DESC_F_NEXT;
	disk_queue.desc[0].next = 1;

	disk_queue.desc[1].addr = (uintptr_t)data;
	disk_queue.desc[1].len = 512;
	disk_queue.desc[1].flags =
		VIRTQ_DESC_F_NEXT |
		(type == VIRTIO_BLK_T_IN ? VIRTQ_DESC_F_WRITE : 0);
	disk_queue.desc[1].next = 2;

	disk_queue.desc[2].addr = (uintptr_t)&disk_io.status;
	disk_queue.desc[2].len = 1;
	disk_queue.desc[2].flags = VIRTQ_DESC_F_WRITE;
	disk_queue.desc[2].next = 0;

	const u16 last_used = disk_queue.used.idx;

	virtio_mb();

	disk_queue.avail.ring[disk_queue.avail.idx % QSIZE] = 0;
	disk_queue.avail.idx++;

	virtio_mb();
	vio_write(VIRTIO_MMIO_QUEUE_NOTIFY_OFFSET, 0);

	while (disk_queue.used.idx == last_used)
		virtio_mb();

	if (disk_io.status != 0)
		panic("virtio_blk: I/O error (type=%u sector=%lu status=%u)",
		      type, sector, (u32)disk_io.status);
}

void blkread(u64 no, char *data)
{
	const u64 first_sector = no * (BLKSZ / 512);

	for (u32 i = 0; i < BLKSZ / 512; i++)
		virtio_blk_submit(VIRTIO_BLK_T_IN, first_sector + i,
				  data + i * 512);
}

void blkwrite(u64 no, char *data)
{
	const u64 first_sector = no * (BLKSZ / 512);

	for (u32 i = 0; i < BLKSZ / 512; i++)
		virtio_blk_submit(VIRTIO_BLK_T_OUT, first_sector + i,
				  data + i * 512);
}
