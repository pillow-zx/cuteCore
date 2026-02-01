#include <stddef.h>
#include <stdint.h>

#include "arch/riscv/riscv.h"
#include "utils/klib.h"

#define BLOCK_FREE_BIT (1 << 0)
#define BLOCK_PREV_FREE_BIT (1 << 1)

#define SLI 4
#define L1_COUNT 32
#define L2_COUNT (1 << SLI)

#define ALIGN_SIZE 8

struct block_header_t {
    struct block_header_t *prev_phys_block;
    size_t size;
    struct block_header_t *next_free;
    struct block_header_t *prev_free;
};

struct tlsf_control_t {
    uint32_t fl_bitmap;
    uint32_t sf_bitmap[L1_COUNT];
    struct block_header_t *blocks[L1_COUNT][L2_COUNT];
};

static struct tlsf_control_t *global_tlsf_control = NULL;

__always_inline int32_t tlsf_fls(uint32_t word) { return word == 0 ? -1 : 31 - __builtin_clz(word); }

__always_inline int32_t tlsf_ffs(uint32_t word) { return word == 0 ? -1 : __builtin_ctz(word); }

__always_inline void mapping(size_t size, int32_t *fl, int32_t *sl) {
    if (size < 128) {
        *fl = 0;
        *sl = (int32_t)size / (128 / L2_COUNT);
    } else {
        *fl = tlsf_fls((uint32_t)size);
        *sl = (int32_t)(size >> (*fl - SLI)) - L2_COUNT;
    }
}

static struct block_header_t *find_suitable_block(struct tlsf_control_t *ctrl, int32_t *fl, int32_t *sl) {
    if (*fl < 0 || *fl >= L1_COUNT) {
        return NULL;
    }
    uint32_t sl_map = ctrl->sf_bitmap[*fl] & (~0U << *sl);

    if (sl_map == 0) {
        uint32_t fl_map = ctrl->fl_bitmap & (~0U << (*fl + 1));
        if (fl_map == 0) { return NULL; }

        *fl = tlsf_ffs(fl_map);
        sl_map = ctrl->sf_bitmap[*fl];
    }

    *sl = tlsf_ffs(sl_map);
    return ctrl->blocks[*fl][*sl];
}

static void insert_block_to_list(struct tlsf_control_t *ctrl, struct block_header_t *block, int32_t fl, int32_t sl) {
    struct block_header_t *head = ctrl->blocks[fl][sl];

    block->next_free = head;
    block->prev_free = NULL;

    if (head) { head->prev_free = block; }

    ctrl->blocks[fl][sl] = block;

    ctrl->fl_bitmap |= (1U << fl);
    ctrl->sf_bitmap[fl] |= (1U << sl);
}

static void remove_block_from_list(struct tlsf_control_t *ctrl, struct block_header_t *block, int32_t fl, int32_t sl) {
    struct block_header_t *prev = block->prev_free;
    struct block_header_t *next = block->next_free;

    if (next) { next->prev_free = prev; }

    if (prev) {
        prev->next_free = next;
    } else {
        ctrl->blocks[fl][sl] = next;

        if (next == NULL) {
            ctrl->sf_bitmap[fl] &= ~(1U << sl);
            if (ctrl->sf_bitmap[fl] == 0) { ctrl->fl_bitmap &= ~(1U << fl); }
        }
    }
}

static void remove_block(struct tlsf_control_t *ctrl, struct block_header_t *block) {
    int32_t fl, sl;
    mapping(block->size & ~3, &fl, &sl);
    remove_block_from_list(ctrl, block, fl, sl);
}

static void insert_block(struct tlsf_control_t *ctrl, struct block_header_t *block) {
    int32_t fl, sl;
    mapping(block->size & ~3, &fl, &sl);
    insert_block_to_list(ctrl, block, fl, sl);
}

static void split_block(struct tlsf_control_t *ctrl, struct block_header_t *block, size_t size) {
    struct block_header_t *remaining = (struct block_header_t *)((char *)block + sizeof(struct block_header_t) + size);
    size_t remaining_size = (block->size & ~3) - size - sizeof(struct block_header_t);

    if (remaining_size >= 32) {
        remaining->size = remaining_size | BLOCK_FREE_BIT;
        remaining->prev_phys_block = (struct block_header_t *)block;

        block->size = size | (block->size & 2);
        insert_block(ctrl, remaining);
    }
}

static struct block_header_t *merge_prev(struct tlsf_control_t *ctrl, struct block_header_t *block) {
    if (block->size & BLOCK_PREV_FREE_BIT) {
        struct block_header_t *prev = block->prev_phys_block;
        remove_block(ctrl, prev);
        prev->size += (block->size & ~3) + sizeof(struct block_header_t);
        block = prev;
    }
    return block;
}

static struct block_header_t *block_next(struct block_header_t *block) {
    return (struct block_header_t *)((char *)block + sizeof(struct block_header_t) + (block->size & ~3));
}

static struct block_header_t *merge_next(struct tlsf_control_t *ctrl, struct block_header_t *block) {
    struct block_header_t *next = block_next(block);
    if (next->size & BLOCK_FREE_BIT) {
        remove_block(ctrl, next);
        block->size += (next->size & ~3) + sizeof(struct block_header_t);
    }
    return block;
}

static void *kmalloc_helper(struct tlsf_control_t *ctrl, size_t size) {
    size_t adjust_size = (size + ALIGN_SIZE - 1) & ~(ALIGN_SIZE - 1);
    if (adjust_size < 32) { adjust_size = 32; }

    int32_t fl, sl;
    mapping(adjust_size, &fl, &sl);

    struct block_header_t *block = find_suitable_block(ctrl, &fl, &sl);
    if (!block) { return NULL; }

    remove_block(ctrl, block);
    split_block(ctrl, block, adjust_size);

    block->size &= ~BLOCK_FREE_BIT;
    struct block_header_t *next = block_next(block);
    next->size &= ~BLOCK_PREV_FREE_BIT;

    return (void *)((char *)block + sizeof(struct block_header_t));
}

static void kfree_helper(struct tlsf_control_t *ctrl, void *ptr) {
    if (!ptr) { return; }

    struct block_header_t *block = (struct block_header_t *)((char *)ptr - sizeof(struct block_header_t));

    block->size |= BLOCK_FREE_BIT;

    block = merge_prev(ctrl, block);
    block = merge_next(ctrl, block);

    struct block_header_t *next = block_next(block);
    if (next->size & BLOCK_FREE_BIT) {
        remove_block(ctrl, next);
        block->size += (next->size & ~3) + sizeof(struct block_header_t);
        next = block_next(block);
    }

    next->size |= BLOCK_PREV_FREE_BIT;
    next->prev_phys_block = block;

    insert_block(ctrl, block);
}

static void *krealloc_helper(struct tlsf_control_t *ctrl, void *ptr, size_t size) {
    if (!ptr) { return kmalloc_helper(ctrl, size); }

    struct block_header_t *block = (struct block_header_t *)((char *)ptr - sizeof(struct block_header_t));
    size_t old_size = block->size & ~3;
    size_t adjust_size = (size + ALIGN_SIZE - 1) & ~(ALIGN_SIZE - 1);

    if (adjust_size <= old_size) {
        split_block(ctrl, block, adjust_size);
        return ptr;
    }

    struct block_header_t *next = block_next(block);
    if ((next->size & BLOCK_FREE_BIT) &&
        (old_size + sizeof(struct block_header_t *) + (next->size & ~3) >= adjust_size)) {
        remove_block(ctrl, next);
        block->size += (next->size & ~3) + sizeof(struct block_header_t);
        split_block(ctrl, block, adjust_size);
        return ptr;
    }

    void *new_ptr = kmalloc_helper(ctrl, size);
    if (!new_ptr) { return NULL; }
    memcpy(new_ptr, ptr, old_size);

    kfree_helper(ctrl, ptr);

    return new_ptr;
}

static void ktlsf_init(struct tlsf_control_t *ctrl, void *mem, size_t bytes) {
    ctrl = (struct tlsf_control_t *)mem;

    size_t pool_overhead = sizeof(struct tlsf_control_t) + sizeof(struct block_header_t) * 2;
    size_t big_size = (bytes - pool_overhead) & ~(ALIGN_SIZE - 1);

    struct block_header_t *null_block = (struct block_header_t *)((char *)ctrl + sizeof(struct tlsf_control_t));
    null_block->size = 0 | 0;
    null_block->prev_phys_block = NULL;

    struct block_header_t *big_block = (struct block_header_t *)((char *)null_block + sizeof(struct block_header_t));
    big_block->size = big_size | BLOCK_FREE_BIT;
    big_block->prev_phys_block = null_block;

    struct block_header_t *tail =
        (struct block_header_t *)((char *)big_block + sizeof(struct block_header_t) + big_size);
    tail->size = 0 | 0;
    tail->size &= ~BLOCK_PREV_FREE_BIT;
    tail->prev_phys_block = big_block;

    tail->size |= BLOCK_PREV_FREE_BIT;

    insert_block(ctrl, big_block);
}

void tlsf_init(void *mem, size_t size) { ktlsf_init(global_tlsf_control, mem, size); }

void *kmalloc(size_t size) {
    if (size == 0 || !global_tlsf_control) { return NULL; }

    intr_off();
    void *ptr = kmalloc_helper(global_tlsf_control, size);
    intr_on();

    return ptr;
}

void *krealloc(void *ptr, size_t size) {
    if (!global_tlsf_control) { return NULL; }

    intr_off();
    void *new_ptr = krealloc_helper(global_tlsf_control, ptr, size);
    intr_on();

    return new_ptr;
}

void kfree(void *ptr) {
    if (!ptr || !global_tlsf_control) { return; }

    intr_off();
    kfree_helper(global_tlsf_control, ptr);
    intr_on();
}
