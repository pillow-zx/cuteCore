#include "mm/kalloc.h"
#include "config.h"
#include "log.h"
#include "list.h"
#include "utils.h"

void freerange(void *pa_start, void *pa_end);

struct {
	struct list_head *memlist;
} kmem;

extern char ekernel[];

void kinit(void)
{
	freerange(ekernel, (void *)PHYEND);
	kmallocinit();
	Log("Pmem init success");
}

void freerange(void *pa_start, void *pa_end)
{
	for (auto p = (char *)PGROUNDUP((uint64_t)pa_start);
	     p + PGSIZE <= (char *)pa_end; p += PGSIZE) {
		kfree(p);
	}
}

void kfree(void *pa)
{
	if (((u64)pa % PGSIZE) != 0 || (char *)pa < ekernel ||
	    (u64)pa >= PHYEND) {
		panic("kfree");
	}

	const auto head = (struct list_head *)pa;

	head->next = kmem.memlist;
	kmem.memlist = head;
}

void *kalloc(void)
{
	struct list_head *head = kmem.memlist;
	if (head) {
		kmem.memlist = head->next;
		memset((char *)head, 0, PGSIZE);
	}
	return head;
}
