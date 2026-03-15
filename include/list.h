#ifndef LIST_H
#define LIST_H

#include "utils.h"

struct list_head {
	struct list_head *next;
	struct list_head *prev;
};

#define LIST_HEAD_INIT(name) {&(name), &(name)}
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

__always_inline void list_init(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

__always_inline void list_add(struct list_head *node, struct list_head *head)
{
	head->next->prev = node;
	node->next = head->next;
	node->prev = head;
	head->next = node;
}

__always_inline void list_add_tail(struct list_head *node,
				   struct list_head *head)
{
	head->prev->next = node;
	node->next = head;
	node->prev = head->prev;
	head->prev = node;
}

__always_inline void list_del(struct list_head *node)
{
	node->next->prev = node->prev;
	node->prev->next = node->next;
}

__always_inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

__always_inline struct list_head *list_first(const struct list_head *head)
{
	return head->next;
}

__always_inline struct list_head *list_last(const struct list_head *head)
{
	return head->prev;
}

#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member)))

#define list_for_each(pos, head) \
	for ((pos) = (head)->next; (pos) != (head); (pos) = (pos)->next)

#define list_for_each_safe(pos, n, head)                               \
	for ((pos) = (head)->next, (n) = (pos)->next; (pos) != (head); \
	     (pos) = (n), (n) = (pos)->next)

#endif
