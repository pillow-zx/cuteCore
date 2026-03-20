#include "kernel/task.h"
#include "fs.h"
#include "riscv.h"

void scheduler(void)
{
	struct proc *proc = nullptr;
	struct cpu *cpu = curcpu();

	for (;;) {
		intr_off();
		if (ready_list_head != nullptr) {
			bsync();
			proc = ready_list_head;
			ready_list_head = proc->next;
			if (ready_list_head)
				ready_list_head->prev = nullptr;
			else
				ready_list_tail = nullptr;

			proc->state = RUNNING;

			cpu->proc = proc;

			swtch(&cpu->context, &proc->context);

			cpu->proc = nullptr;

		} else {
			bsync();
			intr_on();
			asm volatile("wfi");
		}
	}
}
