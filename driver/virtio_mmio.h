#ifndef VIRTIO_MMIO_H
#define VIRTIO_MMIO_H

#include "utils.h"
#include "config.h"

#define VIRTIO_MMIO_BASE VIRTIO_BLK0

#define VIRTIO_MMIO_MAGIC_OFFSET 0x000
#define VIRTIO_MMIO_VERSION_OFFSET 0x004
#define VIRTIO_MMIO_DEVICEID_OFFSET 0x008
#define VIRTIO_MMIO_VENDORID_OFFSET 0x00c

#define VIRTIO_MMIO_DEVICE_FEATURES_OFFSET 0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL_OFFSET 0x014
#define VIRTIO_MMIO_DRIVER_FEATURES_OFFSET 0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL_OFFSET 0x024

#define VIRTIO_MMIO_QUEUE_SEL_OFFSET 0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX_OFFSET 0x034
#define VIRTIO_MMIO_QUEUE_NUM_OFFSET 0x038
#define VIRTIO_MMIO_QUEUE_READY_OFFSET 0x044

#define VIRTIO_MMIO_QUEUE_NOTIFY_OFFSET 0x050

#define VIRTIO_MMIO_INT_STATUS_OFFSET 0x060
#define VIRTIO_MMIO_INT_ACK_OFFSET 0x064

#define VIRTIO_MMIO_STATUS_OFFSET 0x070

#define VIRTIO_MMIO_QUEUE_DESC_LOW_OFFSET 0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH_OFFSET 0x084
#define VIRTIO_MMIO_QUEUE_DRIVER_LOW_OFFSET 0x090
#define VIRTIO_MMIO_QUEUE_DRIVER_HIGH_OFFSET 0x094
#define VIRTIO_MMIO_QUEUE_DEVICE_LOW_OFFSET 0x0a0
#define VIRTIO_MMIO_QUEUE_DEVICE_HIGH_OFFSET 0x0a4

#define VIRTIO_MMIO_REG(reg) ((volatile u32 *)(VIRTIO_MMIO_BASE + (reg)))

#define VIRTIO_MMIO_MAGIC_VALUE 0x74726976u
#define VIRTIO_MMIO_VERSION 2u
#define VIRTIO_MMIO_DEVICEID_BLK 2u

#define VIRTIO_STATUS_ACKNOWLEDGE 0x01u
#define VIRTIO_STATUS_DRIVER 0x02u
#define VIRTIO_STATUS_DRIVER_OK 0x04u
#define VIRTIO_STATUS_FEATURES_OK 0x08u
#define VIRTIO_STATUS_FAILED 0x80u

#define VIRTIO_F_VERSION_1 (1u << 0)

#define VIRTQ_DESC_F_NEXT 0x1u
#define VIRTQ_DESC_F_WRITE 0x2u

#define QSIZE 8u

struct virtq_desc {
	u64 addr;
	u32 len;
	u16 flags;
	u16 next;
} __packed;

struct virtq_avail {
	u16 flags;
	u16 idx;
	u16 ring[QSIZE];
	u16 used_event;
} __packed;

struct virtq_used_elem {
	u32 id;
	u32 len;
} __packed;

struct virtq_used {
	u16 flags;
	u16 idx;
	struct virtq_used_elem ring[QSIZE];
	u16 avail_event;
} __packed;

#define VIRTQ_AVAIL_OFFSET (sizeof(struct virtq_desc) * QSIZE)
#define VIRTQ_PAD_SIZE \
	(PGSIZE - VIRTQ_AVAIL_OFFSET - sizeof(struct virtq_avail))

struct virtqueue {
	struct virtq_desc desc[QSIZE];
	struct virtq_avail avail;
	u8 _pad[VIRTQ_PAD_SIZE];
	struct virtq_used used;
} __packed;

#define VIRTIO_BLK_T_IN 0u
#define VIRTIO_BLK_T_OUT 1u

struct virt_blk_req {
	u32 type;
	u32 reserved;
	u64 sector;
} __packed;

struct virtio_blk_io {
	struct virt_blk_req hdr;
	volatile u8 status;
};

#endif
