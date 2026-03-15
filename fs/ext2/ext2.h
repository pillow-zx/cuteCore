#ifndef EXT2_H
#define EXT2_H

#include "ext2_def.h"

extern struct inode_ops ext2_inode_ops;

extern struct file_ops ext2_file_ops;

i32 ext2_read_inode(struct inode *inode);

i32 ext2_write_inode(struct inode *inode);

void ext2_destroy_inode(struct inode *inode);

struct inode *ext2_lookup(struct inode *dir, const char *name, i32 namelen);

struct inode *ext2_create(struct inode *dir, const char *name, i32 namelen,
			  u32 mode);

struct inode *ext2_mkdir(struct inode *dir, const char *name, i32 namelen,
			 u32 mode);

i32 ext2_unlink(struct inode *dir, const char *name, i32 namelen);

i32 ext2_file_open(struct inode *inode, struct file *file);

void ext2_file_release(struct inode *inode, struct file *file);

i32 ext2_file_read(struct file *file, void *buf, usize len, off_t *offset);

i32 ext2_file_write(struct file *file, const void *buf, usize len,
		    off_t *offset);

struct inode *ext2_iget(u32 ino);

void ext2_inode_loc(u32 ino, u32 *blk_no, u32 *offset);

void ext2_truncate_inode(struct inode *inode);

u32 ext2_alloc_block(void);
void ext2_free_block(u32 blk_no);

u32 ext2_alloc_inode(void);
void ext2_free_inode(u32 ino);

#endif
