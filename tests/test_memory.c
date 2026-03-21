/**
 * test_memory.c — Unit tests for ae_arena and ae_pool
 */
#include "ae_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int passed = 0;
static int failed = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { printf("  PASS: %s\n", msg); ++passed; } \
        else      { printf("  FAIL: %s  (line %d)\n", msg, __LINE__); ++failed; } \
    } while (0)

/* =========================================================================
 * Arena tests
 * ========================================================================= */

static void test_arena_basic(void) {
    printf("[arena] basic alloc\n");
    ae_arena_t a;
    CHECK(ae_arena_init(&a, 4096) == AE_OK, "init ok");

    void *p1 = ae_arena_alloc(&a, 64, 8);
    CHECK(p1 != NULL, "alloc 64 bytes");

    void *p2 = ae_arena_alloc(&a, 128, 16);
    CHECK(p2 != NULL, "alloc 128 bytes");
    CHECK((uintptr_t)p2 % 16 == 0, "alignment 16");

    /* Write and read back */
    memset(p1, 0xAB, 64);
    CHECK(*(uint8_t *)p1 == 0xAB, "write-back ok");

    ae_arena_destroy(&a);
    printf("[arena] basic alloc done\n\n");
}

static void test_arena_reset(void) {
    printf("[arena] reset\n");
    ae_arena_t a;
    ae_arena_init(&a, 256);

    for (int i = 0; i < 10; ++i)
        ae_arena_alloc(&a, 100, 1);

    ae_arena_reset(&a);
    CHECK(a.total == 0, "total reset to 0");

    void *p = ae_arena_alloc(&a, 64, 1);
    CHECK(p != NULL, "alloc after reset");

    ae_arena_destroy(&a);
    printf("[arena] reset done\n\n");
}

static void test_arena_overflow(void) {
    printf("[arena] overflow into new block\n");
    ae_arena_t a;
    ae_arena_init(&a, 256);

    /* Allocate more than one block worth */
    int ok = 1;
    for (int i = 0; i < 20; ++i) {
        void *p = ae_arena_alloc(&a, 64, 8);
        if (!p) { ok = 0; break; }
    }
    CHECK(ok, "20 * 64-byte allocs succeed (multi-block)");

    ae_arena_destroy(&a);
    printf("[arena] overflow done\n\n");
}

/* =========================================================================
 * Pool tests
 * ========================================================================= */

typedef struct { int x, y, z; double w; } my_obj_t;

static void test_pool_basic(void) {
    printf("[pool] basic alloc/free\n");
    ae_pool_t p;
    CHECK(ae_pool_init(&p, sizeof(my_obj_t), 16) == AE_OK, "init ok");

    my_obj_t *a = (my_obj_t *)ae_pool_alloc(&p);
    CHECK(a != NULL, "alloc 1");
    CHECK(a->x == 0 && a->y == 0, "zeroed");

    a->x = 42; a->y = 7;
    ae_pool_free(&p, a);

    /* After free the slot is recycled */
    my_obj_t *b = (my_obj_t *)ae_pool_alloc(&p);
    CHECK(b != NULL, "reuse after free");
    /* b is zeroed on reuse (ae_pool_alloc zeroes) */
    CHECK(b->x == 0, "zeroed on reuse");

    ae_pool_destroy(&p);
    printf("[pool] basic done\n\n");
}

static void test_pool_stress(void) {
    printf("[pool] stress 1000 alloc/free\n");
    ae_pool_t pool;
    ae_pool_init(&pool, sizeof(my_obj_t), 256);

    my_obj_t *ptrs[1000];
    for (int i = 0; i < 1000; ++i) {
        ptrs[i] = (my_obj_t *)ae_pool_alloc(&pool);
        CHECK(ptrs[i] != NULL, "alloc in loop");
        ptrs[i]->x = i;
    }
    for (int i = 0; i < 1000; ++i)
        ae_pool_free(&pool, ptrs[i]);

    CHECK(pool.alloc_cnt == 0, "all freed");
    ae_pool_destroy(&pool);
    printf("[pool] stress done\n\n");
}

/* =========================================================================
 * Main
 * ========================================================================= */

int main(void) {
    printf("=== Memory Tests ===\n\n");

    test_arena_basic();
    test_arena_reset();
    test_arena_overflow();
    test_pool_basic();
    test_pool_stress();

    printf("=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed ? 1 : 0;
}
