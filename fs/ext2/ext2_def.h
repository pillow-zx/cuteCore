#ifndef EXT2_DEF_H
#define EXT2_DEF_H

#include "fs.h"
#include "utils.h"

#define EXT2_SUPER_BLKNUM 0
#define EXT2_SUPER_OFFSET 1024
#define EXT2_GDESC_OFFSET 2048
#define EXT2_SUPER_MAGIC 0xEF53
#define EXT2_GOOD_OLD_INODE_SIZE 128
#define EXT2_ROOT_INO 2

#define EXT2_GOOD_OLD_REV 0
#define EXT2_DYNAMIC_REV 1

#define EXT2_MAX_LOG_BLOCK_SIZE 2

#define EXT2_NDIR_BLOCKS 12
#define EXT2_IND_BLOCK EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK (EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK (EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS (EXT2_TIND_BLOCK + 1)

struct ext2_bg_desc {
	u32 bg_block_bitmap;
	u32 bg_inode_bitmap;
	u32 bg_inode_table;
	u16 bg_free_blocks_count;
	u16 bg_free_inodes_count;
	u16 bg_used_dirs_count;
	u16 bg_pad;
	u32 bg_reserved[3];
} __packed;

struct ext2_inode {
	u16 i_mode;
	u16 i_uid;
	u32 i_size;
	u32 i_atime;
	u32 i_ctime;
	u32 i_mtime;
	u32 i_dtime;
	u16 i_gid;
	u16 i_links_count;
	u32 i_blocks;
	u32 i_flags;
	u32 i_reserved1;
	u32 i_block[EXT2_N_BLOCKS];
	u32 i_generation;
	u32 i_file_acl;
	u32 i_dir_acl;
	u32 i_faddr;

	u8 i_frag;
	u8 i_fsize;
	u16 i_pad;
	u16 i_uid_high;
	u16 i_gid_high;
	u32 i_reserved2;
} __packed;

struct ext2_super_block {
	u32 s_inode_count;
	u32 s_blocks_count;
	u32 s_r_blocks_count;
	u32 s_free_blocks_count;
	u32 s_free_inodes_count;
	u32 s_first_data_block;
	u32 s_log_block_size;
	u32 s_log_frag_size;
	u32 s_blocks_per_group;
	u32 s_frags_per_group;
	u32 s_inodes_per_group;
	u32 s_mtime;
	u32 s_wtime;
	u16 s_mnt_count;
	u16 s_max_mnt_count;
	u16 s_magic;
	u16 s_state;
	u16 s_errors;
	u16 s_minor_rev_level;
	u32 s_lastcheck;
	u32 s_checkinterval;
	u32 s_creator_os;
	u32 s_rev_level;
	u16 s_def_resuid;
	u16 s_def_resgid;

	u32 s_first_ino;
	u16 s_inode_size;
	u16 s_block_group_nr;
	u32 s_feature_compat;
	u32 s_feature_incompat;
	u32 s_feature_ro_compat;
	u8 s_uuid[16];
	char s_volume_name[16];
	char s_last_mounted[64];
	u32 s_algorithm_usage_bitmap;

	u8 s_prealloc_blocks;
	u8 s_prealloc_dir_blocks;
	u16 s_padding1;

	u8 s_journal_uuid[16];
	u32 s_journal_inum;
	u32 s_journal_dev;
	u32 s_last_orphan;
	u32 s_hash_seed[4];
	u8 s_def_hash_version;
	u8 s_reserved_char_pad;
	u16 s_reserved_word_pad;
	u32 s_default_mount_opts;
	u32 s_first_meta_bg;
	u32 s_reserved[190];
} __packed;

struct ext2_dir_entry {
	u32 inode;
	u16 rec_len;
	u8 name_len;
	u8 file_type;
	char name[];
} __packed;

struct ext2_sb_info {
	struct ext2_super_block *raw_sb;
	struct ext2_bg_desc *gb_table;
	u32 group_count;
	u32 inode_size;
	u32 inode_per_group;
	struct blk_cache *sb_buf;
};

struct ext2_inode_info {
	u32 i_block[EXT2_N_BLOCKS];
	u32 i_flags;
};

#endif
