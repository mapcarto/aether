/**
 * ae_memory.h — Pool allocator and arena allocator for aetoer
 *
 * Two allocator types are provided:
 *  - ae_arena_t : bump-pointer arena for batch/frame allocations; O(1) alloc,
 *                 O(1) bulk-free.
 *  - ae_pool_t  : fixed-size slab pool for hot-path objects; O(1) alloc/free
 *                 with no fragmentation.
 *
 * Neither allocator calls malloc/free on the hot path after initialisation.
 */
#ifndef AETOER_MEMORY_H
#define AETOER_MEMORY_H

#include "ae_types.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Arena allocator
 * ========================================================================= */

typedef struct ae_arena_block ae_arena_block_t;

struct ae_arena_block {
    ae_arena_block_t *next;
    size_t            capacity;
    size_t            used;
    /* data follows immediately after the struct */
};

typedef struct {
    ae_arena_block_t *head;     /* current (newest) block              */
    size_t            block_sz; /* default block size for new blocks   */
    size_t            total;    /* total bytes allocated across blocks */
} ae_arena_t;

/**
 * Initialise an arena.  @block_sz is the allocation granularity for backing
 * blocks (must be > 0; rounded up to at least 4 KiB internally).
 */
ae_result_t ae_arena_init(ae_arena_t *a, size_t block_sz);

/** Release all memory owned by the arena back to the OS. */
void ae_arena_destroy(ae_arena_t *a);

/** Reset the arena so all memory can be reused without returning it to OS. */
void ae_arena_reset(ae_arena_t *a);

/**
 * Allocate @size bytes from the arena, aligned to @align bytes.
 * Returns NULL only when the OS refuses to give more memory.
 */
void *ae_arena_alloc(ae_arena_t *a, size_t size, size_t align);

#define AE_ARENA_ALLOC_T(arena, T) \
    ((T *)ae_arena_alloc((arena), sizeof(T), _Alignof(T)))

#define AE_ARENA_ALLOC_N(arena, T, n) \
    ((T *)ae_arena_alloc((arena), sizeof(T) * (n), _Alignof(T)))


/* =========================================================================
 * Pool allocator
 * ========================================================================= */

typedef struct ae_pool_free_node {
    struct ae_pool_free_node *next;
} ae_pool_free_node_t;

typedef struct {
    ae_arena_t           arena;
    ae_pool_free_node_t *free_list;
    size_t               obj_size;  /* size of each object (>= sizeof(ptr)) */
    size_t               alloc_cnt; /* live allocations */
} ae_pool_t;

/**
 * Initialise a pool that hands out objects of @obj_size bytes, backed by an
 * arena with @initial_cap pre-allocated objects.
 */
ae_result_t ae_pool_init(ae_pool_t *p, size_t obj_size, size_t initial_cap);

/** Destroy pool and release all backing memory. */
void ae_pool_destroy(ae_pool_t *p);

/** Obtain one object from the pool (zeroed). */
void *ae_pool_alloc(ae_pool_t *p);

/** Return one object to the pool. */
void ae_pool_free(ae_pool_t *p, void *obj);

#ifdef __cplusplus
}
#endif

#endif /* AETOER_MEMORY_H */
