#include "kernel.h"
#include "riscv.h"

void scheduler(void)
{
	struct proc *proc = nullptr;
	struct cpu *cpu = curcpu();

	for (;;) {
		if (ready_list_head != nullptr) {
			bsync();
			proc = ready_list_head;
			ready_list_head = proc->next;
			if (ready_list_head)
				ready_list_head->prev = nullptr;
			else
				ready_list_tail = nullptr;

			proc->state = RUNNING;

			swtch(&cpu->context, &proc->context);

			cpu->proc = proc;
			cpu->context = proc->context;

		} else {
			bsync();
			intr_on();
			asm volatile("wfi");
		}
	}
}
