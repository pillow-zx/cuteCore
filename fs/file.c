/**
 * @file file.c
 * @brief 全局文件表管理实现
 *
 * 实现打开文件对象的分配、释放、引用计数管理。
 * 当前为单核设计，预留锁扩展接口。
 */

#include "fs.h"
#include "list.h"
#include "log.h"
#include "utils.h"

/** @brief UART 设备操作表（定义于 driver/uart.c） */
extern struct file_ops uart_ops;

/** @brief file 结构体的 slab 缓存 */
static struct cache *file_cache;

/** @brief 全局打开文件表 */
struct ftable ftable;

/**
 * @brief 初始化全局文件表
 */
void fileinit(void)
{
	list_init(&ftable.head);
	ftable.count = 0;

	file_cache = cache_create(sizeof(struct file));
	if (file_cache == nullptr)
		panic("fileinit: failed to create file cache");

	Log("File table init success");
}

/**
 * @brief 分配一个新的打开文件对象
 *
 * @return 新的打开文件对象
 */
struct file *filealloc(void)
{
	struct file *f = cache_alloc(file_cache);
	if (f == nullptr)
		return nullptr;

	/* 初始化字段 */
	f->ref = 1;
	f->flags = 0;
	f->offset = 0;
	f->inode = nullptr;
	f->ops = nullptr;

	/* 初始化链表节点 */
	list_init(&f->f_link);

	/* 加入全局文件表 */
	list_add_tail(&f->f_link, &ftable.head);
	ftable.count++;

	return f;
}

/**
 * @brief 关闭文件并释放引用
 *
 * @param f 目标文件
 */
void fileclose(struct file *f)
{
	if (f == nullptr)
		return;

	if (--f->ref > 0)
		return;

	/* ref == 0，真正关闭 */
	if (f->ops && f->ops->release)
		f->ops->release(f->inode, f);

	/* 释放 inode 引用 */
	if (f->inode)
		iput(f->inode);

	/* 从全局文件表移除 */
	list_del(&f->f_link);
	ftable.count--;

	/* 释放 file 结构内存 */
	cache_free(file_cache, f);
}

/**
 * @brief 增加文件引用计数
 *
 * @param f 目标文件
 *
 * @return 目标文件
 */
struct file *filedup(struct file *f)
{
	if (f == nullptr)
		return nullptr;

	f->ref++;
	return f;
}

/**
 * @brief 获取当前打开文件数量
 *
 * @return 当前打开文件数量
 */
usize filecount(void)
{
	return ftable.count;
}

/**
 * @brief 从打开的文件读取数据
 *
 * 根据文件类型分发到对应的读取实现：
 * - INODE_FILE: 读取普通文件
 * - INODE_DEV: 调用设备读取函数
 */
i32 fileread(struct file *f, void *buf, usize len)
{
	if (f == nullptr || f->inode == nullptr)
		return -1;

	if (!FILE_READABLE(f))
		return -1;

	struct inode *ip = f->inode;

	switch (ip->type) {
	case INODE_FILE: {
		i32 ret = readi(ip, buf, f->offset, len);
		if (ret > 0)
			f->offset += ret;
		return ret;
	}

	case INODE_DEV: {
		struct file_ops *ops = f->ops ? f->ops : &uart_ops;
		if (ops->read == nullptr)
			return -1;
		off_t off = f->offset;
		return ops->read(f, buf, len, &off);
	}

	default:
		return -1;
	}
}

/**
 * @brief 向打开的文件写入数据
 *
 * 根据文件类型分发到对应的写入实现：
 * - INODE_FILE: 写入普通文件
 * - INODE_DEV: 调用设备写入函数
 */
i32 filewrite(struct file *f, const void *buf, usize len)
{
	if (f == nullptr || f->inode == nullptr)
		return -1;

	if (!FILE_WRITABLE(f))
		return -1;

	struct inode *ip = f->inode;

	switch (ip->type) {
	case INODE_FILE: {
		i32 ret = writei(ip, buf, f->offset, len);
		if (ret > 0)
			f->offset += ret;
		return ret;
	}

	case INODE_DEV: {
		struct file_ops *ops = f->ops ? f->ops : &uart_ops;
		if (ops->write == nullptr)
			return -1;
		off_t off = f->offset;
		return ops->write(f, buf, len, &off);
	}

	default:
		return -1;
	}
}
