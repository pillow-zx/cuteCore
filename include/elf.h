/**
 * @file elf.h
 * @brief ELF64 文件格式定义
 *
 * 本文件定义了 ELF (Executable and Linkable Format) 64 位文件格式的
 * 相关常量和数据结构。ELF 是 Unix/Linux 系统中可执行文件、目标文件
 * 和共享库的标准格式。
 *
 */

#ifndef ELF_H
#define ELF_H

#include "types.h"

/** @brief ELF 文件头标识字段长度 */
#define EI_NIDENT 16
/** @brief e_ident 索引：Magic 字节 0 (0x7f) */
#define EI_MAG0 0
/** @brief e_ident 索引：Magic 字节 1 ('E') */
#define EI_MAG1 1
/** @brief e_ident 索引：Magic 字节 2 ('L') */
#define EI_MAG2 2
/** @brief e_ident 索引：Magic 字节 3 ('F') */
#define EI_MAG3 3
/** @brief e_ident 索引：文件类别（32/64 位） */
#define EI_CLASS 4
/** @brief e_ident 索引：数据编码（字节序） */
#define EI_DATA 5
/** @brief e_ident 索引：ELF 版本 */
#define EI_VERSION 6
/** @brief e_ident 索引：操作系统/ABI 标识 */
#define EI_OSABI 7
/** @brief e_ident 索引：ABI 版本 */
#define EI_ABIVERSION 8
/** @brief e_ident 索引：填充起始位置 */
#define EI_PAD 9

/** @brief ELF Magic 字节 0 */
#define ELFMAG0 0x7f
/** @brief ELF Magic 字节 1 */
#define ELFMAG1 'E'
/** @brief ELF Magic 字节 2 */
#define ELFMAG2 'L'
/** @brief ELF Magic 字节 3 */
#define ELFMAG3 'F'
/** @brief ELF Magic 字符串 "\177ELF" */
#define ELFMAG "\177ELF"
/** @brief ELF Magic 长度 */
#define SELFMAG 4

/** @brief 文件类别：无效 */
#define ELFCLASSNONE 0
/** @brief 文件类别：32 位 */
#define ELFCLASS32 1
/** @brief 文件类别：64 位 */
#define ELFCLASS64 2

/** @brief 数据编码：无效 */
#define ELFDATANONE 0
/** @brief 数据编码：小端序 (Little Endian) */
#define ELFDATA2LSB 1
/** @brief 数据编码：大端序 (Big Endian) */
#define ELFDATA2MSB 2

/** @brief ELF 版本：无效 */
#define EV_NONE 0
/** @brief ELF 版本：当前版本 */
#define EV_CURRENT 1

/** @brief 操作系统 ABI：未指定 */
#define ELFOSABI_NONE 0
/** @brief 操作系统 ABI：Linux */
#define ELFOSABI_LINUX 3
/** @brief 操作系统 ABI：FreeBSD */
#define ELFOSABI_FREEBSD 9

/** @brief 文件类型：无效 */
#define ET_NONE 0
/** @brief 文件类型：可重定位文件 (.o) */
#define ET_REL 1
/** @brief 文件类型：可执行文件 */
#define ET_EXEC 2
/** @brief 文件类型：共享目标文件 (.so) */
#define ET_DYN 3
/** @brief 文件类型：核心转储文件 (core) */
#define ET_CORE 4
/** @brief 文件类型：操作系统特定范围起始 */
#define ET_LOOS 0xfe00
/** @brief 文件类型：操作系统特定范围结束 */
#define ET_HIOS 0xfeff
/** @brief 文件类型：处理器特定范围起始 */
#define ET_LOPROC 0xff00
/** @brief 文件类型：处理器特定范围结束 */
#define ET_HIPROC 0xffff

/** @brief 机器架构：未指定 */
#define EM_NONE 0
/** @brief 机器架构：Intel x86 */
#define EM_386 3
/** @brief 机器架构：ARM */
#define EM_ARM 40
/** @brief 机器架构：AMD x86-64 */
#define EM_X86_64 62
/** @brief 机器架构：AArch64 (ARM 64-bit) */
#define EM_AARCH64 183
/** @brief 机器架构：RISC-V */
#define EM_RISCV 243

/**
 * @defgroup ElfProgramType ELF 程序头类型
 * @brief 段类型，用于识别段的用途
 * @{
 */
/** @brief 段类型：未使用 */
#define PT_NULL 0
/** @brief 段类型：可加载段（需加载到内存） */
#define PT_LOAD 1
/** @brief 段类型：动态链接信息 */
#define PT_DYNAMIC 2
/** @brief 段类型：解释器路径名 */
#define PT_INTERP 3
/** @brief 段类型：注释信息 */
#define PT_NOTE 4
/** @brief 段类型：程序头表自身 */
#define PT_PHDR 6
/** @} */

/**
 * @defgroup ElfProgramFlags ELF 程序头标志
 * @brief 段权限标志，用于设置内存页属性
 * @{
 */
/** @brief 段标志：可执行 */
#define PF_X 0x1
/** @brief 段标志：可写 */
#define PF_W 0x2
/** @brief 段标志：可读 */
#define PF_R 0x4
/** @} */

/**
 * @defgroup ElfSectionType ELF 节类型
 * @brief 节类型，用于识别节的用途
 * @{
 */
/** @brief 节类型：未使用 */
#define SHT_NULL 0
/** @brief 节类型：程序定义的内容（代码或数据） */
#define SHT_PROGBITS 1
/** @brief 节类型：符号表 */
#define SHT_SYMTAB 2
/** @brief 节类型：字符串表 */
#define SHT_STRTAB 3
/** @brief 节类型：重定位项（有显式加数） */
#define SHT_RELA 4
/** @brief 节类型：符号哈希表 */
#define SHT_HASH 5
/** @brief 节类型：动态链接信息 */
#define SHT_DYNAMIC 6
/** @brief 节类型：注释信息 */
#define SHT_NOTE 7
/** @brief 节类型：未初始化数据（.bss，不占文件空间） */
#define SHT_NOBITS 8
/** @brief 节类型：重定位项（无显式加数） */
#define SHT_REL 9
/** @brief 节类型：动态链接符号表 */
#define SHT_DYNSYM 11
/** @} */

/**
 * @defgroup ElfSectionFlags ELF 节标志
 * @brief 节属性标志
 * @{
 */
/** @brief 节标志：可写 */
#define SHF_WRITE 0x1
/** @brief 节标志：占用内存空间（加载时需要分配） */
#define SHF_ALLOC 0x2
/** @brief 节标志：包含可执行指令 */
#define SHF_EXECINSTR 0x4
/** @} */

/**
 * @brief ELF64 文件头
 *
 * 描述 ELF 文件的整体信息，包括文件类型、目标架构、入口地址等。
 * 位于文件起始位置。
 */
struct Elf64_Ehdr {
	uchar e_ident[EI_NIDENT]; /**< ELF 标识字节数组 */
	u16 e_type;               /**< 文件类型 (ET_*) */
	u16 e_machine;            /**< 目标机器架构 (EM_*) */
	u32 e_version;            /**< ELF 版本 (EV_CURRENT) */
	u64 e_entry;              /**< 程序入口点虚拟地址 */
	u64 e_phoff;              /**< 程序头表文件偏移 */
	u64 e_shoff;              /**< 节头表文件偏移 */
	u32 e_flags;              /**< 处理器特定标志 */
	u16 e_ehsize;             /**< ELF 文件头大小 */
	u16 e_phentsize;          /**< 程序头表项大小 */
	u16 e_phnum;              /**< 程序头表项数量 */
	u16 e_shentsize;          /**< 节头表项大小 */
	u16 e_shnum;              /**< 节头表项数量 */
	u16 e_shstrndx;           /**< 节名字符串表在节头表中的索引 */
};

/**
 * @brief ELF64 程序头
 *
 * 描述一个可加载段的信息，用于加载程序到内存。
 */
struct Elf64_Phdr {
	u32 p_type;   /**< 段类型 (PT_LOAD 等) */
	u32 p_flags;  /**< 段标志 (可读/可写/可执行) */
	u64 p_offset; /**< 段在文件中的偏移 */
	u64 p_vaddr;  /**< 段的虚拟地址 */
	u64 p_paddr;  /**< 段的物理地址（保留） */
	u64 p_filesz; /**< 段在文件中的大小 */
	u64 p_memsz;  /**< 段在内存中的大小 */
	u64 p_align;  /**< 段对齐要求 */
};

/**
 * @brief ELF64 节头
 *
 * 描述 ELF 文件中的一个节（section），用于链接和调试信息。
 */
struct Elf64_Shdr {
	u32 sh_name;      /**< 节名称在节字符串表中的索引 */
	u32 sh_type;      /**< 节类型 (SHT_PROGBITS 等) */
	u64 sh_flags;     /**< 节标志 (可写、可分配等) */
	u64 sh_addr;      /**< 节的虚拟地址（若加载到内存） */
	u64 sh_offset;    /**< 节在文件中的偏移 */
	u64 sh_size;      /**< 节的大小 */
	u64 sh_link;      /**< 相关节的索引 */
	u64 sh_info;      /**< 附加信息 */
	u64 sh_addralign; /**< 对齐要求 */
	u64 sh_entsize;   /**< 固定大小表项的大小（若有） */
};

/**
 * @brief 检查 ELF Magic 是否有效
 *
 * @param ehdr 指向 ELF 文件头的指针
 *
 * @return 非零值表示 Magic 有效，0 表示无效
 */
#define ELF_MAGIC_OK(ehdr)                      \
	((ehdr)->e_ident[EI_MAG0] == ELFMAG0 && \
	 (ehdr)->e_ident[EI_MAG1] == ELFMAG1 && \
	 (ehdr)->e_ident[EI_MAG2] == ELFMAG2 && \
	 (ehdr)->e_ident[EI_MAG3] == ELFMAG3)

/**
 * @brief 检查 ELF 文件是否为 RISC-V 架构
 *
 * @param ehdr 指向 ELF 文件头的指针
 *
 * @return 非零值表示是 RISC-V 架构，0 表示其他架构
 */
#define ELF_IS_RISCV(ehdr) ((ehdr)->e_machine == EM_RISCV)

#endif
