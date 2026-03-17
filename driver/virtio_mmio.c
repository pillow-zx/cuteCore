/**
 * @file virtio_mmio.c
 * @brief VirtIO MMIO 块设备驱动
 *
 * 本文件实现了 VirtIO 1.2 规范的 MMIO 传输层块设备驱动。
 * 驱动使用轮询方式（非中断）处理 I/O 请求，支持 512 字节扇区的读写操作。
 */

#include "driver.h"
#include "log.h"
#include "virtio_mmio.h"

/** @brief VirtIO 请求队列，对齐到块大小 */
struct virtqueue disk_queue __aligned(BLKSZ);

/** @brief 磁盘 I/O 请求结构体（含头部、数据缓冲区、状态字节） */
static struct virtio_blk_io disk_io;

/**
 * @brief 读取 VirtIO MMIO 寄存器
 *
 * @param reg 寄存器偏移量
 * @return 寄存器值
 */
__always_inline u32 vio_read(u32 reg)
{
	return *VIRTIO_MMIO_REG(reg);
}

/**
 * @brief 写入 VirtIO MMIO 寄存器
 *
 * @param reg 寄存器偏移量
 * @param val 待写入的值
 */
__always_inline void vio_write(u32 reg, u32 val)
{
	*VIRTIO_MMIO_REG(reg) = val;
}

#define virtio_mb() __asm__ volatile("fence ow, ir" ::: "memory")

/**
 * @brief VirtIO MMIO 块设备初始化
 *
 * 按照 VirtIO 1.2 规范完成设备初始化流程：
 * 1. 身份校验（magic number、version、device ID）
 * 2. 重置设备状态
 * 3. 特征协商（协商支持的设备特征）
 * 4. 配置虚拟队列（描述符表、可用环、已用环）
 * 5. 激活设备
 *
 * @note 初始化失败将触发 panic
 */
void blkinit(void)
{
	// 1. 身份校验

	// 校验 magic number
	const u32 magic = vio_read(VIRTIO_MMIO_MAGIC_OFFSET);
	if (magic != VIRTIO_MMIO_MAGIC_VALUE)
		panic("blkinit: bad magic 0x%x", magic);

	// 校验 version
	const u32 version = vio_read(VIRTIO_MMIO_VERSION_OFFSET);
	if (version != VIRTIO_MMIO_VERSION)
		panic("blkinit: unsupported version %u (want 2)", version);

	// 校验块设备id
	const u32 device_id = vio_read(VIRTIO_MMIO_DEVICEID_OFFSET);
	if (device_id != VIRTIO_MMIO_DEVICEID_BLK)
		panic("blkinit: unexpected device id %u", device_id);

	// 2. 重置状态
	vio_write(VIRTIO_MMIO_STATUS_OFFSET, 0);
	while (vio_read(VIRTIO_MMIO_STATUS_OFFSET) != 0)
		;

	// 3. 特征协调
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

	// 4. 配置虚拟队列
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

	// 5. 激活设备
	vio_write(VIRTIO_MMIO_STATUS_OFFSET,
		  VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
			  VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK);

	Log("Virtioblk mmio init success");
}

/**
 * @brief 构造并提交磁盘 I/O 请求
 *
 * 构造包含三段描述符的请求链：
 * - desc[0]: 请求头（类型、扇区号）
 * - desc[1]: 数据缓冲区（512 字节）
 * - desc[2]: 状态字节（设备写入操作结果）
 *
 * 使用轮询方式等待请求完成。
 *
 * @param type 请求类型（VIRTIO_BLK_T_IN 读 / VIRTIO_BLK_T_OUT 写）
 * @param sector 目标扇区号（512 字节为单位）
 * @param data 数据缓冲区指针
 *
 * @note 当前实现为同步阻塞，未来可改用中断提高效率
 * @note 硬编码描述符索引，一次只能处理一个请求
 */
static void virtio_blk_submit(u32 type, u64 sector, char *data)
{
	// TODO: 这里的逻辑是同步的，未来可以使用中断代替轮询，提高效率;
	// TODO: 硬编码desc使得一次只能处理一个请求，未来可以通过动态管理QSIZE来支持高并发

	// desc[0](Header)：包括操作类型和扇区号
	// desc[1](Data)：512bytes数据缓冲区
	// desc[2](Status)：预留一个字节给设备，成功0，失败非0
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

	// 轮询等待，检查设备是否处理完整
	while (disk_queue.used.idx == last_used)
		virtio_mb();

	if (disk_io.status != 0)
		panic("virtio_blk: I/O error (type=%u sector=%lu status=%u)",
		      type, sector, (u32)disk_io.status);
}

/**
 * @brief 读取一个完整块
 *
 * 将一个 BLKSZ 大小的块读取到指定缓冲区。
 * 由于设备以 512 字节扇区为单位操作，需要多次调用 virtio_blk_submit。
 *
 * @param no 目标块号（BLKSZ 大小的块）
 * @param data 数据缓冲区指针（至少 BLKSZ 字节）
 */
void blkread(u64 no, char *data)
{
	const u64 first_sector = no * (BLKSZ / 512);

	for (u32 i = 0; i < BLKSZ / 512; i++)
		virtio_blk_submit(VIRTIO_BLK_T_IN, first_sector + i,
				  data + i * 512);
}

/**
 * @brief 写入一个完整块
 *
 * 将指定缓冲区的数据写入一个 BLKSZ 大小的块。
 * 由于设备以 512 字节扇区为单位操作，需要多次调用 virtio_blk_submit。
 *
 * @param no 目标块号（BLKSZ 大小的块）
 * @param data 数据缓冲区指针（至少 BLKSZ 字节）
 */
void blkwrite(u64 no, char *data)
{
	const u64 first_sector = no * (BLKSZ / 512);

	for (u32 i = 0; i < BLKSZ / 512; i++)
		virtio_blk_submit(VIRTIO_BLK_T_OUT, first_sector + i,
				  data + i * 512);
}
