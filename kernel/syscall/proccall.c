#include "config.h"
#include "elf.h"
#include "fs.h"
#include "kernel/task.h"
#include "mm/kalloc.h"
#include "mm/vm.h"
#include "riscv.h"
#include "types.h"
#include "utils.h"

/**
 * @brief 在用户栈上构造 argc/argv
 *
 * 按照 RISC-V ABI 规范在用户栈上布局参数：
 * - 字符串从栈顶向下存储
 * - 指针数组紧随其后（16字节对齐）
 * - NULL 终止符结尾
 *
 * @param pagetable 用户页表
 * @param sp        栈顶地址（输入/输出参数）
 * @param argv      参数数组（内核空间）
 * @param argc_out  输出参数，返回 argc 值
 * @param argv_out  输出参数，返回用户空间 argv 地址
 *
 * @return 成功返回 0，失败返回 -1
 */
static bool setup_argv(pagetable_t pagetable, u64 *sp, char **argv,
		       i32 *argc_out, u64 *argv_out)
{
	if (argv == nullptr) {
		*argc_out = 0;
		*argv_out = 0;
		return true;
	}

	/* 计算 argc 和字符串总长度 */
	i32 argc = 0;
	while (argv[argc] != nullptr) {
		argc++;
	}

	if (argc == 0) {
		*argc_out = 0;
		*argv_out = 0;
		return true;
	}

	/* 分配临时缓冲区存储字符串指针 */
	u64 *arg_ptrs = kmalloc(argc * sizeof(u64));
	if (arg_ptrs == nullptr)
		return false;

	/* 拷贝字符串到栈顶（从高地址向低地址） */
	u64 string_base = *sp;
	for (i32 i = argc - 1; i >= 0; i--) {
		u64 len = strlen(argv[i]) + 1;
		string_base -= len;

		if (!copyout(pagetable, string_base, argv[i], len)) {
			kmfree(arg_ptrs);
			return false;
		}

		arg_ptrs[i] = string_base; /* 记录字符串地址 */
	}

	/* 16 字节对齐 */
	string_base = ALIGNDOWN(string_base, 16);

	/* 构造 argv 数组（指针数组 + NULL 终止符） */
	u64 argv_array_size = (argc + 1) * sizeof(u64);
	u64 argv_base = string_base - argv_array_size;

	for (i32 i = 0; i < argc; i++) {
		if (!copyout(pagetable, argv_base + i * sizeof(u64),
			     (char *)&arg_ptrs[i], sizeof(u64))) {
			kmfree(arg_ptrs);
			return false;
		}
	}

	/* NULL 终止符 */
	u64 null_ptr = 0;
	if (!copyout(pagetable, argv_base + argc * sizeof(u64),
		     (char *)&null_ptr, sizeof(u64))) {
		kmfree(arg_ptrs);
		return false;
	}

	kmfree(arg_ptrs);

	/* 更新栈指针和返回值 */
	*sp = argv_base;
	*argc_out = argc;
	*argv_out = argv_base;

	return true;
}

/**
 * @brief 加载并执行 ELF 可执行文件
 *
 * 完整流程：
 * 1. 解析路径获取 inode
 * 2. 创建新进程并分配页表
 * 3. 读取并验证 ELF 头
 * 4. 加载所有 PT_LOAD 段到进程内存
 * 5. 分配用户栈
 * 6. 设置 argc/argv
 * 7. 配置 trapframe（入口点、栈指针、参数寄存器）
 *
 * @param path 可执行文件路径
 * @param argv 命令行参数数组（以 NULL 结尾）
 *
 * @return 成功返回 0；失败返回 -1
 */
i32 kexec(const char *path, char **argv)
{
	struct inode *ip = namei(path);
	if (ip == nullptr || ip->type != INODE_FILE) {
		if (ip != nullptr)
			iput(ip);
		return -1;
	}

	struct proc *p = allocproc();
	if (p == nullptr) {
		iput(ip);
		return -1;
	}

	p->root = mapproc();
	if (p->root == nullptr) {
		freeproc(p);
		iput(ip);
		return -1;
	}

	/* 读取并验证 ELF 头 */
	struct Elf64_Ehdr ehdr;
	if (readi(ip, &ehdr, 0, sizeof(ehdr)) != sizeof(ehdr)) {
		freeproc(p);
		iput(ip);
		return -1;
	}

	if (!ELF_MAGIC_OK(&ehdr) || !ELF_IS_RISCV(&ehdr) ||
	    ehdr.e_type != ET_EXEC) {
		freeproc(p);
		iput(ip);
		return -1;
	}

	/* 读取程序头表 */
	usize phdr_size = ehdr.e_phnum * sizeof(struct Elf64_Phdr);
	struct Elf64_Phdr *phdr_table = kmalloc(phdr_size);
	if (phdr_table == nullptr) {
		freeproc(p);
		iput(ip);
		return -1;
	}

	if (readi(ip, phdr_table, ehdr.e_phoff, phdr_size) != (i32)phdr_size) {
		kmfree(phdr_table);
		freeproc(p);
		iput(ip);
		return -1;
	}

	/* 加载程序段 */
	u64 max_va = 0;

	for (u16 i = 0; i < ehdr.e_phnum; i++) {
		struct Elf64_Phdr *ph = &phdr_table[i];

		if (ph->p_type != PT_LOAD || ph->p_memsz == 0)
			continue;

		vaddr_t va = ALIGNDOWN(ph->p_vaddr, PGSIZE);
		vaddr_t va_end = ALIGNUP(ph->p_vaddr + ph->p_memsz, PGSIZE);

		if (va_end > max_va)
			max_va = va_end;

		/* 分配物理页并建立映射 */
		for (vaddr_t v = va; v < va_end; v += PGSIZE) {
			paddr_t pa = (paddr_t)kalloc();
			if (pa == 0)
				goto fail;

			u64 perm = PTE_V | PTE_U;
			if (ph->p_flags & PF_R)
				perm |= PTE_R;
			if (ph->p_flags & PF_W)
				perm |= PTE_W;
			if (ph->p_flags & PF_X)
				perm |= PTE_X;

			if (!vmmap(p->root, v, PGSIZE, pa, perm))
				goto fail;
		}

		/* 从文件读取段数据 */
		if (ph->p_filesz > 0) {
			char *seg_buf = kmalloc(ph->p_filesz);
			if (seg_buf == nullptr)
				goto fail;

			if (readi(ip, seg_buf, ph->p_offset, ph->p_filesz) !=
			    (i32)ph->p_filesz) {
				kmfree(seg_buf);
				goto fail;
			}

			if (!copyout(p->root, ph->p_vaddr, seg_buf,
				     ph->p_filesz)) {
				kmfree(seg_buf);
				goto fail;
			}

			kmfree(seg_buf);
		}
		/* .bss 段已由 kalloc() 清零 */
	}

	/* 分配用户栈 */
	const u64 stack_size = 2 * PGSIZE;
	vaddr_t stack_top = KERNBASE;
	vaddr_t stack_bottom = stack_top - stack_size;

	if (max_va > stack_bottom)
		goto fail;

	for (vaddr_t v = stack_bottom; v < stack_top; v += PGSIZE) {
		paddr_t pa = (paddr_t)kalloc();
		if (pa == 0)
			goto fail;

		if (!vmmap(p->root, v, PGSIZE, pa,
			   PTE_R | PTE_W | PTE_U | PTE_V))
			goto fail;
	}

	p->stack_bottom = stack_bottom;
	p->stack_top = stack_top;

	/* 设置 argv */
	u64 sp = stack_top;
	i32 argc = 0;
	u64 argv_ptr = 0;

	if (!setup_argv(p->root, &sp, argv, &argc, &argv_ptr))
		goto fail;

	/* 设置 trapframe */
	p->trapframe->sepc = ehdr.e_entry;
	p->trapframe->sp = sp;
	p->trapframe->a0 = argc;
	p->trapframe->a1 = argv_ptr;

	/* 更新进程信息 */
	p->sz = max_va;
	p->brk = max_va;

	usize name_len = strlen(path);
	if (name_len >= sizeof(p->name))
		name_len = sizeof(p->name) - 1;
	memcpy(p->name, path, name_len);
	p->name[name_len] = '\0';

	/* 清空文件表 */
	memset(p->fdtable, 0, sizeof(p->fdtable));

	kmfree(phdr_table);
	iput(ip);

	return 0;

fail:
	kmfree(phdr_table);
	freeproc(p);
	iput(ip);
	return -1;
}

i64 sys_execve(void)
{
	return 0;
}
