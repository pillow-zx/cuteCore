#include "fs.h"
#include "log.h"
#include "./ext2/ext2.h"

struct vfs fs;

/**
 * @brief 文件系统 VFS 初始化
 *
 */
void fsinit(void)
{
	if (ext2_fs_init() < 0) {
		panic("Vfs init falied");
	}
	Log("Vfs init success");
}

/**
 * @brief 释放 inode 的引用，当引用计数为0时可能触发删除或写回
 *
 * @param inode 指向目标 inode 的指针
 */
void iput(struct inode *inode)
{
	if (inode == nullptr)
		return;

	// 释放对 inode 的一次引用
	inode->ref--;

	if (inode->ref > 0)
		return;

	// 如果 inode 为 fs 的 root，设置应用次数为1，确保不会释放
	if (inode == fs.root) {
		inode->ref = 1;
		return;
	}

	// 如果 inode 的硬链接数为0可以删除
	if (inode->nlinks == 0) {
		ext2_truncate_inode(inode); // 清空 inode 数据块
		ext2_free_inode(inode->ino); // 回收索引
	} else {
		if (inode->dirty) // 如果文件已经被修改则写回
			inode->iops->write_inode(inode);
	}

	// 从 inode 缓存中链表中移除
	list_del(&inode->cache_link);
	fs.icache_size--;

	// 销毁 inode
	inode->iops->destroy_inode(inode);
}

/**
 * @brief 跳过输入绝对路径开头的所有 /
 *
 * @param path 绝对路径字符串
 * @return const char* 去除开头 / 后的路径字符串
 */
static const char *skipslash(const char *path)
{
	while (*path == '/')
		path++;
	return path;
}

/**
 * @brief 从 `path` 中提取下一个路径分量并存入 `name`，返回剩余路径
 *
 * @param path 路径字符串
 * @param name 保存路径分量
 * @return const char* 提取下一个路径分量后的剩余路径字符串
 */
static const char *path_next(const char *path, char *name)
{
	if (*path == '\0')
		return nullptr;

	const char *start = path;
	while (*path != '/' && *path != '\0')
		path++;

	usize len = (usize)(path - start);
	if (len >= MAXPATHLEN)
		len = MAXPATHLEN - 1;
	memcpy(name, start, len);
	name[len] = '\0';

	return path;
}

/**
 * @brief 路径解析函数，通过字符串路径逐级解析得到最终的inode
 *
 * @param path 绝对字符串路径
 * @param parent 是否只返回父目录的inode（而非目标本身）
 * @param namebuf 如果只返回父目录，把最后一个路径分量的名字放到这里
 * @return struct inode* 对最终inode的指针
 */
static struct inode *namex(const char *path, const bool parent, char *namebuf)
{
	// 1. 校验并从根目录出发

	if (path == nullptr || *path != '/')
		return nullptr;

	struct inode *cur = fs.root; // 从根目录 / 开始
	if (cur == nullptr)
		return nullptr;
	cur->ref++; // 增加引用计数，表示有一个新进程在用这个inode

	path = skipslash(path + 1); // 跳过开头的 /和多余的斜杠

	// 2. 逐级解析路径分量
	char name[MAXPATHLEN];

	while (true) {
		const char *rest = path_next(path, name);

		// 情况一：路径已经走到最后
		if (rest == nullptr) {
			if (parent) {
				iput(cur); // 要求获得父目录但路径只有 /， 无意义
				return nullptr;
			}
			return cur; // 返回当前的inode
		}

		// 情况二：parent模式 + 已到达最后一段
		rest = skipslash(rest);

		if (parent && *rest == '\0') {
			if (namebuf != nullptr) {
				// 把最后一个分量的名字写入namebuf
				usize l = strlen(name);
				if (l >= MAXPATHLEN)
					l = MAXPATHLEN - 1;
				memcpy(namebuf, name, l + 1);
			}
			return cur; // 返回父目录 inode
		}

		// 情况三：中间分量，继续查找
		if (cur->type != INODE_DIR) { // 如果中间节点是一个目录
			iput(cur);
			return nullptr;
		}

		struct inode *next =
			cur->iops->lookup(cur, name, (i32)strlen(name));
		iput(cur); // 释放对当前 inode 的引用

		if (next == nullptr)
			return nullptr;

		cur = next; // 前往下一级
		path = rest; // 继续处理剩余路径
	}
}

/**
 * @brief 解析完整路径，返回目标 inode
 *
 * @param path 完整绝对路径
 * @return struct inode* 目标 inode
 */
struct inode *namei(const char *path)
{
	return namex(path, false, nullptr);
}

/**
 * @brief 解析完整路径，返回父目录 inode，并把最后一段的名字写入 name
 *
 * @param path 完整绝对路径
 * @param name 最后一段名字
 * @return struct inode* 父目录 inode
 */
struct inode *nameiparent(const char *path, char *name)
{
	return namex(path, true, name);
}
