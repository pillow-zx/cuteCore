/**
 * @file virtio_mmio.h
 * @brief VirtIO MMIO 块设备驱动头文件
 *
 * 本文件定义了 VirtIO 1.2 规范中 MMIO 传输层的相关常量、寄存器偏移量
 * 以及虚拟队列的数据结构。适用于 VirtIO 块设备的驱动实现。
 */

#ifndef VIRTIO_MMIO_H
#define VIRTIO_MMIO_H

#include "utils.h"
#include "config.h"

/** @brief VirtIO MMIO 设备基地址 */
#define VIRTIO_MMIO_BASE VIRTIO_BLK0

/** @brief Magic Value 寄存器偏移量，用于身份校验 */
#define VIRTIO_MMIO_MAGIC_OFFSET 0x000
/** @brief Version 寄存器偏移量，指示设备版本 */
#define VIRTIO_MMIO_VERSION_OFFSET 0x004
/** @brief Device ID 寄存器偏移量，指示设备类型 */
#define VIRTIO_MMIO_DEVICEID_OFFSET 0x008
/** @brief Vendor ID 寄存器偏移量，指示厂商 ID */
#define VIRTIO_MMIO_VENDORID_OFFSET 0x00c

/** @brief Device Features 寄存器偏移量，读取设备支持的特征 */
#define VIRTIO_MMIO_DEVICE_FEATURES_OFFSET 0x010
/** @brief Device Features Select 寄存器偏移量，选择特征位图的高/低 32 位 */
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL_OFFSET 0x014
/** @brief Driver Features 寄存器偏移量，写入驱动接受的特征 */
#define VIRTIO_MMIO_DRIVER_FEATURES_OFFSET 0x020
/** @brief Driver Features Select 寄存器偏移量，选择特征位图的高/低 32 位 */
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL_OFFSET 0x024

/** @brief Queue Select 寄存器偏移量，选择要操作的虚拟队列 */
#define VIRTIO_MMIO_QUEUE_SEL_OFFSET 0x030
/** @brief Queue Num Max 寄存器偏移量，读取队列最大描述符数量 */
#define VIRTIO_MMIO_QUEUE_NUM_MAX_OFFSET 0x034
/** @brief Queue Num 寄存器偏移量，设置队列描述符数量 */
#define VIRTIO_MMIO_QUEUE_NUM_OFFSET 0x038
/** @brief Queue Ready 寄存器偏移量，指示队列是否就绪 */
#define VIRTIO_MMIO_QUEUE_READY_OFFSET 0x044

/** @brief Queue Notify 寄存器偏移量，通知设备处理队列 */
#define VIRTIO_MMIO_QUEUE_NOTIFY_OFFSET 0x050

/** @brief Interrupt Status 寄存器偏移量，读取中断状态 */
#define VIRTIO_MMIO_INT_STATUS_OFFSET 0x060
/** @brief Interrupt Ack 寄存器偏移量，确认中断 */
#define VIRTIO_MMIO_INT_ACK_OFFSET 0x064

/** @brief Device Status 寄存器偏移量，控制设备状态机 */
#define VIRTIO_MMIO_STATUS_OFFSET 0x070

/** @brief Queue Descriptor Low 寄存器偏移量，描述符表地址低 32 位 */
#define VIRTIO_MMIO_QUEUE_DESC_LOW_OFFSET 0x080
/** @brief Queue Descriptor High 寄存器偏移量，描述符表地址高 32 位 */
#define VIRTIO_MMIO_QUEUE_DESC_HIGH_OFFSET 0x084
/** @brief Queue Driver Low 寄存器偏移量，可用环地址低 32 位 */
#define VIRTIO_MMIO_QUEUE_DRIVER_LOW_OFFSET 0x090
/** @brief Queue Driver High 寄存器偏移量，可用环地址高 32 位 */
#define VIRTIO_MMIO_QUEUE_DRIVER_HIGH_OFFSET 0x094
/** @brief Queue Device Low 寄存器偏移量，已用环地址低 32 位 */
#define VIRTIO_MMIO_QUEUE_DEVICE_LOW_OFFSET 0x0a0
/** @brief Queue Device High 寄存器偏移量，已用环地址高 32 位 */
#define VIRTIO_MMIO_QUEUE_DEVICE_HIGH_OFFSET 0x0a4

/**
 * @brief 访问 VirtIO MMIO 寄存器
 *
 * @param reg 寄存器偏移量
 *
 * @return 指向寄存器的 volatile 指针
 */
#define VIRTIO_MMIO_REG(reg) ((volatile u32 *)(VIRTIO_MMIO_BASE + (reg)))

/** @brief VirtIO Magic Value，应为 "virt" 的 ASCII 码 (0x74726976) */
#define VIRTIO_MMIO_MAGIC_VALUE 0x74726976u
/** @brief VirtIO MMIO 版本号，本驱动要求版本 2 */
#define VIRTIO_MMIO_VERSION 2u
/** @brief VirtIO 块设备 ID */
#define VIRTIO_MMIO_DEVICEID_BLK 2u

/** @brief 设备状态：已确认设备存在 */
#define VIRTIO_STATUS_ACKNOWLEDGE 0x01u
/** @brief 设备状态：驱动已绑定 */
#define VIRTIO_STATUS_DRIVER 0x02u
/** @brief 设备状态：驱动已就绪 */
#define VIRTIO_STATUS_DRIVER_OK 0x04u
/** @brief 设备状态：特征协商成功 */
#define VIRTIO_STATUS_FEATURES_OK 0x08u
/** @brief 设备状态：初始化失败 */
#define VIRTIO_STATUS_FAILED 0x80u

/** @brief 特征位：支持 VirtIO 1.0+ 规范 */
#define VIRTIO_F_VERSION_1 (1u << 0)

/** @brief 描述符标志：链中还有下一个描述符 */
#define VIRTQ_DESC_F_NEXT 0x1u
/** @brief 描述符标志：设备可写入此缓冲区 */
#define VIRTQ_DESC_F_WRITE 0x2u

/** @brief 虚拟队列大小（描述符数量） */
#define QSIZE 8u

/**
 * @brief 虚拟队列描述符
 *
 * 描述一个用于 I/O 传输的内存缓冲区。
 */
struct virtq_desc {
	u64 addr;  /**< 缓冲区物理地址 */
	u32 len;   /**< 缓冲区长度（字节） */
	u16 flags; /**< 描述符标志（VIRTQ_DESC_F_*） */
	u16 next;  /**< 链中下一个描述符的索引（当 flags 含 NEXT 时有效） */
} __packed;

/**
 * @brief 虚拟队列可用环
 *
 * 驱动向设备提交的待处理请求描述符索引列表。
 */
struct virtq_avail {
	u16 flags;       /**< 标志位（当前未使用） */
	u16 idx;         /**< 下一个可用槽位的索引 */
	u16 ring[QSIZE]; /**< 描述符索引数组 */
	u16 used_event;  /**< 事件抑制机制（当前未使用） */
} __packed;

/**
 * @brief 虚拟队列已用环元素
 *
 * 描述一个已由设备处理的请求。
 */
struct virtq_used_elem {
	u32 id;  /**< 已完成的描述符链头索引 */
	u32 len; /**< 设备写入的字节数 */
} __packed;

/**
 * @brief 虚拟队列已用环
 *
 * 设备向驱动返回的已完成请求列表。
 */
struct virtq_used {
	u16 flags;                /**< 标志位（当前未使用） */
	u16 idx;                  /**< 下一个已用槽位的索引 */
	struct virtq_used_elem ring[QSIZE]; /**< 已完成请求数组 */
	u16 avail_event;          /**< 事件抑制机制（当前未使用） */
} __packed;

/** @brief 可用环在 virtqueue 中的偏移量 */
#define VIRTQ_AVAIL_OFFSET (sizeof(struct virtq_desc) * QSIZE)
/**
 * @brief 可用环与已用环之间的填充大小
 *
 * 确保已用环位于独立的页面对齐地址，避免缓存一致性冲突。
 */
#define VIRTQ_PAD_SIZE \
	(PGSIZE - VIRTQ_AVAIL_OFFSET - sizeof(struct virtq_avail))

/**
 * @brief 虚拟队列
 *
 * 包含描述符表、可用环和已用环的完整队列结构。
 * 按 VirtIO 规范要求布局，确保已用环位于独立页面。
 */
struct virtqueue {
	struct virtq_desc desc[QSIZE]; /**< 描述符表 */
	struct virtq_avail avail;      /**< 可用环 */
	u8 _pad[VIRTQ_PAD_SIZE];       /**< 填充，确保已用环页面分离 */
	struct virtq_used used;        /**< 已用环 */
} __packed;

/** @brief 块设备请求类型：读操作 */
#define VIRTIO_BLK_T_IN 0u
/** @brief 块设备请求类型：写操作 */
#define VIRTIO_BLK_T_OUT 1u

/**
 * @brief VirtIO 块设备请求头
 *
 * 每个 I/O 请求的开始部分，指定操作类型和目标扇区。
 */
struct virt_blk_req {
	u32 type;     /**< 请求类型（VIRTIO_BLK_T_IN 或 VIRTIO_BLK_T_OUT） */
	u32 reserved; /**< 保留字段 */
	u64 sector;   /**< 目标扇区号（512 字节为单位） */
} __packed;

/**
 * @brief VirtIO 块设备 I/O 请求
 *
 * 完整的块设备请求，包含请求头和状态字节。
 */
struct virtio_blk_io {
	struct virt_blk_req hdr;   /**< 请求头 */
	volatile u8 status;        /**< 设备写入的状态（0 成功，非 0 失败） */
};

#endif
