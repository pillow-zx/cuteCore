#include "kernel/task.h"
#include "kernel/trap.h"
#include "kernel/syscall.h"
#include "proc.h"
#include "riscv.h"
#include "log.h"

extern char trapvec();

void trapinit(void)
{
	w_csr(stvec, (u64)trapvec);
	Log("Trap init success");
}

void trapret(void)
{
	struct proc *p = curproc();

	w_csr(sepc, p->trapframe->sepc);

	u64 sstatus = r_csr(sstatus);
	sstatus &= ~SSTATUS_SPP;
	sstatus |= SSTATUS_SPIE;
	w_csr(sstatus, sstatus);

	w_csr(sscratch, (u64)p->trapframe + TRAPFRAME_SZ);

	w_csr(satp, MAKESATP(p->root));
	sfence_vma();
}

void usertrap(void)
{
	const u64 stval = r_csr(stval);
	const u64 scause = r_csr(scause);

	struct proc *p = curproc();
	p->trapframe->sepc = r_csr(sepc);

	switch (scause) {
		case UserEnvCall: {
			p->trapframe->sepc += 4;
			intr_on();
			syscall();
			break;
		}
		case StoreFault | StoreGuestPageFault | InstructionFault |
			InstructionPageFault | LoadFault | LoadPageFault: {
			panic("[usertrap]: TODO: Unsupported trap from usertrap: scause = 0x%lx, stval = 0x%lx\n",
			      scause, stval);
			break;
		}
		case IllegalInstruction: {
			panic("[usertrap]: TODO:  Unsupported trap from usertrap: scause = 0x%lx, stval = 0x%lx\n",
			      scause, stval);
			break;
		}
		case SupervisorTimer: {
			panic("[usertrap]: TODO: Unsupported trap from usertrap: scause = 0x%lx, stval = 0x%lx\n",
			      scause, stval);
			break;
		}
		case SupervisorExternal: {
			panic("[usertrap]: TODO: Unsupported trap from usertrap: scause = 0x%lx, stval = 0x%lx\n",
			      scause, stval);
			break;
		}
		default: {
			panic("[usertrap]: Unsupported trap from usertrap: scause = 0x%lx, stval = 0x%lx\n",
			      scause, stval);
			break;
		}
	}
	trapret();
}

void kerneltrap(void)
{
	const u64 stval = r_csr(stval);
	const u64 scause = r_csr(scause);
	switch (scause) {
		case SupervisorExternal: {
			panic("[kerneltrap]: TODO: Unsupported trap from kerneltrap: scause = 0x%lx, stval = 0x%lx\n",
			      scause, stval);
			break;
		}
		case SupervisorTimer: {
			panic("[kerneltrap]: TODO: Unsupported trap from kerneltrap: scause = 0x%lx, stval = 0x%lx\n",
			      scause, stval);
			break;
		}
		default: {
			panic("[kerneltrap]: TODO: Unsupported trap from kerneltrap: scause = 0x%lx, stval = 0x%lx\n",
			      scause, stval);
			break;
		}
	}
}
