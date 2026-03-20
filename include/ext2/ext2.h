/**
 * @file ext2.h
 * @brief Ext2 文件系统接口声明
 *
 * 本头文件声明了 Ext2 文件系统的所有公开接口，包括：
 * - inode 操作（读取、写入、销毁、查找、创建）
 * - 文件操作（打开、关闭、读取、写入）
 * - 块分配与释放
 * - inode 分配与释放
 */

#ifndef EXT2_H
#define EXT2_H

#include "ext2/ext2_def.h"

/** @brief Ext2 inode 操作表（定义于 inode.c） */
extern struct inode_ops ext2_inode_ops;

/** @brief Ext2 文件操作表（定义于 dir.c） */
extern struct file_ops ext2_file_ops;

/**
 * @brief 从磁盘读取 inode 数据
 *
 * @param inode 目标 inode 结构体指针
 *
 * @return 成功返回 0；失败返回 -1
 */
i32 ext2_read_inode(struct inode *inode);

/**
 * @brief 将 inode 数据写入磁盘
 *
 * @param inode 目标 inode 结构体指针
 *
 * @return 成功返回 0；失败返回 -1
 */
i32 ext2_write_inode(struct inode *inode);

/**
 * @brief 销毁 inode 结构体
 *
 * @param inode 目标 inode 结构体指针
 */
void ext2_destroy_inode(struct inode *inode);

/**
 * @brief 在目录中查找指定名称的文件
 *
 * @param dir     目录 inode
 * @param name    文件名
 * @param namelen 文件名长度
 *
 * @return 找到返回 inode 指针；未找到返回 nullptr
 */
struct inode *ext2_lookup(const struct inode *dir, const char *name,
			  i32 namelen);

/**
 * @brief 在目录中创建新文件
 *
 * @param dir     父目录 inode
 * @param name    文件名
 * @param namelen 文件名长度
 * @param mode    文件权限
 *
 * @return 成功返回新 inode；失败返回 nullptr
 */
struct inode *ext2_create(struct inode *dir, const char *name, i32 namelen,
			  u32 mode);

/**
 * @brief 在目录中创建子目录
 *
 * @param dir     父目录 inode
 * @param name    目录名
 * @param namelen 目录名长度
 * @param mode    目录权限
 *
 * @return 成功返回新目录 inode；失败返回 nullptr
 */
struct inode *ext2_mkdir(struct inode *dir, const char *name, i32 namelen,
			 u32 mode);

/**
 * @brief 从目录中删除目录项
 *
 * @param dir     父目录 inode
 * @param name    文件名
 * @param namelen 文件名长度
 *
 * @return 成功返回 0；失败返回 -1
 */
i32 ext2_unlink(struct inode *dir, const char *name, i32 namelen);

/**
 * @brief 打开文件
 *
 * @param inode 文件 inode
 * @param file  文件结构体
 *
 * @return 成功返回 0
 */
i32 ext2_file_open(struct inode *inode, struct file *file);

/**
 * @brief 关闭文件
 *
 * @param inode 文件 inode
 * @param file  文件结构体
 */
void ext2_file_release(struct inode *inode, struct file *file);

/**
 * @brief 读取文件内容
 *
 * @param file   文件结构体
 * @param buf    输出缓冲区
 * @param len    读取长度
 * @param offset 偏移量指针
 *
 * @return 实际读取字节数；失败返回负值
 */
i32 ext2_file_read(const struct file *file, void *buf, usize len,
		   off_t *offset);

/**
 * @brief 写入文件内容
 *
 * @param file   文件结构体
 * @param buf    输入缓冲区
 * @param len    写入长度
 * @param offset 偏移量指针
 *
 * @return 实际写入字节数；失败返回负值
 */
i32 ext2_file_write(struct file *file, const void *buf, usize len,
		    off_t *offset);

/**
 * @brief 获取 inode 对象
 *
 * @param ino inode 号
 *
 * @return 成功返回 inode 指针；失败返回 nullptr
 */
struct inode *ext2_iget(u32 ino);

/**
 * @brief 定位 inode 在磁盘上的物理位置
 *
 * @param ino     inode 号
 * @param blk_no  输出块号
 * @param offset  输出块内偏移
 */
void ext2_inode_loc(u32 ino, u32 *blk_no, u32 *offset);

/**
 * @brief 截断 inode 释放所有磁盘块
 *
 * @param inode 目标 inode
 */
void ext2_truncate_inode(struct inode *inode);

/**
 * @brief 分配一个新块
 *
 * @return 成功返回块号；失败返回 0
 */
u32 ext2_alloc_block(void);

/**
 * @brief 释放一个块
 *
 * @param blk_no 块号
 */
void ext2_free_block(u32 blk_no);

/**
 * @brief 分配一个新 inode
 *
 * @return 成功返回 inode 号；失败返回 0
 */
u32 ext2_alloc_inode(void);

/**
 * @brief 释放一个 inode
 *
 * @param ino inode 号
 */
void ext2_free_inode(u32 ino);

/**
 * @brief 初始化 ext2 fs
 *
 * @return 成功返回 0；失败返回非 0
 */
i32 ext2_fs_init(void);

#endif
