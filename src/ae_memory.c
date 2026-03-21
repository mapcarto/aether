/**
 * ae_memory.c — Pool and arena allocator implementation
 */
#include "ae_memory.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Minimum block size: 4 KiB */
#define MIN_BLOCK_SZ 4096u

/* =========================================================================
 * Helpers
 * ========================================================================= */
static size_t align_up(size_t n, size_t align) {
    return (n + align - 1) & ~(align - 1);
}

/* =========================================================================
 * Arena
 * ========================================================================= */

ae_result_t ae_arena_init(ae_arena_t *a, size_t block_sz) {
    if (!a) return AE_ERR_INVALID;
    if (block_sz < MIN_BLOCK_SZ) block_sz = MIN_BLOCK_SZ;

    a->head     = NULL;
    a->block_sz = block_sz;
    a->total    = 0;
    return AE_OK;
}

static ae_arena_block_t *arena_new_block(size_t block_sz) {
    size_t total = sizeof(ae_arena_block_t) + block_sz;
    ae_arena_block_t *b = (ae_arena_block_t *)malloc(total);
    if (!b) return NULL;
    b->next     = NULL;
    b->capacity = block_sz;
    b->used     = 0;
    return b;
}

void ae_arena_destroy(ae_arena_t *a) {
    if (!a) return;
    ae_arena_block_t *b = a->head;
    while (b) {
        ae_arena_block_t *next = b->next;
        free(b);
        b = next;
    }
    a->head  = NULL;
    a->total = 0;
}

void ae_arena_reset(ae_arena_t *a) {
    if (!a) return;
    /* Walk all blocks and reset used counters; keep the memory. */
    for (ae_arena_block_t *b = a->head; b; b = b->next)
        b->used = 0;
    a->total = 0;
}

void *ae_arena_alloc(ae_arena_t *a, size_t size, size_t align) {
    if (!a || size == 0) return NULL;
    if (align == 0) align = 1;

    /* Try current (head) block.
     * Compute the aligned pointer directly from the base address so the
     * result is correct regardless of the block header's own size. */
    if (a->head) {
        uint8_t *base = (uint8_t *)(a->head + 1);
        uint8_t *ptr  = (uint8_t *)align_up(
                             (uintptr_t)(base + a->head->used), align);
        size_t   off  = (size_t)(ptr - base);
        if (off + size <= a->head->capacity) {
            a->head->used = off + size;
            a->total     += size;
            return ptr;
        }
    }

    /* Allocate a new block big enough to hold the requested size. */
    size_t blk_sz = a->block_sz;
    /* Worst-case padding: up to (align-1) extra bytes for alignment. */
    size_t need   = size + (align > 1 ? align - 1 : 0);
    if (need > blk_sz) blk_sz = need;

    ae_arena_block_t *b = arena_new_block(blk_sz);
    if (!b) return NULL;

    b->next = a->head;
    a->head = b;

    uint8_t *base = (uint8_t *)(b + 1);
    uint8_t *ptr  = (uint8_t *)align_up((uintptr_t)base, align);
    size_t   off  = (size_t)(ptr - base);
    b->used   = off + size;
    a->total += size;
    return ptr;
}

/* =========================================================================
 * Pool
 * ========================================================================= */

ae_result_t ae_pool_init(ae_pool_t *p, size_t obj_size, size_t initial_cap) {
    if (!p || obj_size == 0) return AE_ERR_INVALID;

    /* Each object must hold at least one pointer (for the free list). */
    if (obj_size < sizeof(ae_pool_free_node_t))
        obj_size = sizeof(ae_pool_free_node_t);

    p->obj_size  = obj_size;
    p->alloc_cnt = 0;
    p->free_list = NULL;

    /* Back the pool with an arena. */
    size_t block_sz = obj_size * (initial_cap ? initial_cap : 64);
    ae_result_t r   = ae_arena_init(&p->arena, block_sz);
    if (r != AE_OK) return r;

    /* Pre-populate the free list. */
    for (size_t i = 0; i < initial_cap; ++i) {
        void *obj = ae_arena_alloc(&p->arena, obj_size, _Alignof(max_align_t));
        if (!obj) break;
        ae_pool_free_node_t *n = (ae_pool_free_node_t *)obj;
        n->next      = p->free_list;
        p->free_list = n;
    }

    return AE_OK;
}

void ae_pool_destroy(ae_pool_t *p) {
    if (!p) return;
    ae_arena_destroy(&p->arena);
    p->free_list = NULL;
    p->alloc_cnt = 0;
}

void *ae_pool_alloc(ae_pool_t *p) {
    if (!p) return NULL;

    void *obj;
    if (p->free_list) {
        obj          = (void *)p->free_list;
        p->free_list = p->free_list->next;
    } else {
        obj = ae_arena_alloc(&p->arena, p->obj_size, _Alignof(max_align_t));
        if (!obj) return NULL;
    }

    memset(obj, 0, p->obj_size);
    ++p->alloc_cnt;
    return obj;
}

void ae_pool_free(ae_pool_t *p, void *obj) {
    if (!p || !obj) return;
    ae_pool_free_node_t *n = (ae_pool_free_node_t *)obj;
    n->next      = p->free_list;
    p->free_list = n;
    if (p->alloc_cnt > 0) --p->alloc_cnt;
}
