#include "fs.h"
#include "log.h"
#include "./ext2/ext2.h"

struct vfs fs;

void fsinit(void)
{
	if (ext2_fs_init() < 0) {
		panic("Vfs init falied");
	}
	Log("Vfs init success");
}

void iput(struct inode *inode)
{
	if (inode == nullptr)
		return;

	inode->ref--;

	if (inode->ref > 0)
		return;

	if (inode == fs.root) {
		inode->ref = 1;
		return;
	}

	if (inode->nlinks == 0) {
		ext2_truncate_inode(inode);
		ext2_free_inode(inode->ino);
	} else {
		if (inode->dirty)
			inode->iops->write_inode(inode);
	}

	list_del(&inode->cache_link);
	fs.icache_size--;

	inode->iops->destroy_inode(inode);
}

static const char *skipslash(const char *path)
{
	while (*path == '/')
		path++;
	return path;
}

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

static struct inode *namex(const char *path, bool parent, char *namebuf)
{
	if (path == nullptr || *path != '/')
		return nullptr;

	struct inode *cur = fs.root;
	if (cur == nullptr)
		return nullptr;
	cur->ref++;

	path = skipslash(path + 1);

	char name[MAXPATHLEN];

	while (true) {
		const char *rest = path_next(path, name);

		if (rest == nullptr) {
			if (parent) {
				iput(cur);
				return nullptr;
			}
			return cur;
		}

		rest = skipslash(rest);

		if (parent && *rest == '\0') {
			if (namebuf != nullptr) {
				usize l = strlen(name);
				if (l >= MAXPATHLEN)
					l = MAXPATHLEN - 1;
				memcpy(namebuf, name, l + 1);
			}
			return cur;
		}

		if (cur->type != INODE_DIR) {
			iput(cur);
			return nullptr;
		}

		struct inode *next =
			cur->iops->lookup(cur, name, (i32)strlen(name));
		iput(cur);

		if (next == nullptr)
			return nullptr;

		cur = next;
		path = rest;
	}
}

struct inode *namei(const char *path)
{
	return namex(path, false, nullptr);
}

struct inode *nameiparent(const char *path, char *name)
{
	return namex(path, true, name);
}
