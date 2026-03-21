/**
 * test_spatial.c — Unit tests for the pyramid spatial grid
 */
#include "ae_spatial.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static int passed = 0;
static int failed = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { printf("  PASS: %s\n", msg); ++passed; } \
        else      { printf("  FAIL: %s  (line %d)\n", msg, __LINE__); ++failed; } \
    } while (0)

static bool result_contains(const ae_spatial_result_t *r, ae_entity_t e) {
    for (uint32_t i = 0; i < r->count; ++i)
        if (r->entities[i] == e) return true;
    return false;
}

static void test_init_destroy(void) {
    printf("[spatial] init/destroy\n");
    ae_spatial_t s;
    ae_result_t r = ae_spatial_init(&s, 0, 0, 1000, 1000, 4, 4, 4);
    CHECK(r == AE_OK, "init ok");
    ae_spatial_destroy(&s);
    printf("[spatial] init/destroy done\n\n");
}

static void test_insert_query_basic(void) {
    printf("[spatial] insert + query basic\n");
    ae_spatial_t s;
    ae_spatial_init(&s, 0, 0, 1000, 1000, 4, 4, 4);

    ae_entity_t e1 = ae_entity_make(1, 0);
    ae_entity_t e2 = ae_entity_make(2, 0);
    ae_entity_t e3 = ae_entity_make(3, 0);

    ae_aabb2_t box1 = {100, 100, 150, 150}; /* top-left quadrant region */
    ae_aabb2_t box2 = {500, 500, 550, 550}; /* centre-ish */
    ae_aabb2_t box3 = {900, 900, 950, 950}; /* bottom-right */

    CHECK(ae_spatial_insert(&s, e1, &box1) == AE_OK, "insert e1");
    CHECK(ae_spatial_insert(&s, e2, &box2) == AE_OK, "insert e2");
    CHECK(ae_spatial_insert(&s, e3, &box3) == AE_OK, "insert e3");

    ae_spatial_result_t res;
    ae_spatial_result_init(&res);

    /* Query a region that overlaps only e1 */
    ae_aabb2_t q1 = {50, 50, 200, 200};
    ae_spatial_query(&s, &q1, &res);
    CHECK(result_contains(&res, e1), "e1 found in q1");
    CHECK(!result_contains(&res, e2), "e2 not in q1");
    CHECK(!result_contains(&res, e3), "e3 not in q1");

    ae_spatial_result_clear(&res);

    /* Wide query that should contain all three */
    ae_aabb2_t q2 = {0, 0, 1000, 1000};
    ae_spatial_query(&s, &q2, &res);
    CHECK(result_contains(&res, e1), "e1 in full-world query");
    CHECK(result_contains(&res, e2), "e2 in full-world query");
    CHECK(result_contains(&res, e3), "e3 in full-world query");

    ae_spatial_result_free(&res);
    ae_spatial_destroy(&s);
    printf("[spatial] insert + query basic done\n\n");
}

static void test_remove(void) {
    printf("[spatial] remove\n");
    ae_spatial_t s;
    ae_spatial_init(&s, 0, 0, 1000, 1000, 4, 4, 4);

    ae_entity_t e = ae_entity_make(42, 0);
    ae_aabb2_t  box = {200, 200, 250, 250};

    ae_spatial_insert(&s, e, &box);

    ae_spatial_result_t res;
    ae_spatial_result_init(&res);
    ae_spatial_query(&s, &box, &res);
    CHECK(result_contains(&res, e), "found before remove");
    ae_spatial_result_clear(&res);

    ae_result_t r = ae_spatial_remove(&s, e, &box);
    CHECK(r == AE_OK, "remove ok");

    ae_spatial_query(&s, &box, &res);
    CHECK(!result_contains(&res, e), "not found after remove");

    /* Second remove should return NOT_FOUND */
    r = ae_spatial_remove(&s, e, &box);
    CHECK(r == AE_ERR_NOT_FOUND, "double remove => NOT_FOUND");

    ae_spatial_result_free(&res);
    ae_spatial_destroy(&s);
    printf("[spatial] remove done\n\n");
}

static void test_circle_query(void) {
    printf("[spatial] circle query\n");
    ae_spatial_t s;
    ae_spatial_init(&s, 0, 0, 1000, 1000, 4, 4, 4);

    ae_entity_t e_near = ae_entity_make(10, 0);
    ae_entity_t e_far  = ae_entity_make(20, 0);

    ae_aabb2_t near_box = {490, 490, 510, 510}; /* near centre 500,500 */
    ae_aabb2_t far_box  = {800, 800, 820, 820}; /* far from 500,500    */

    ae_spatial_insert(&s, e_near, &near_box);
    ae_spatial_insert(&s, e_far,  &far_box);

    ae_spatial_result_t res;
    ae_spatial_result_init(&res);

    ae_spatial_query_circle(&s, 500, 500, 50, &res);
    CHECK(result_contains(&res, e_near), "near entity found in circle");
    CHECK(!result_contains(&res, e_far), "far entity not in circle");

    ae_spatial_result_free(&res);
    ae_spatial_destroy(&s);
    printf("[spatial] circle query done\n\n");
}

static void test_multi_level(void) {
    printf("[spatial] multi-level (large + small entities)\n");
    ae_spatial_t s;
    /* 5-level pyramid, base 4x4 → finest is 4*(1<<4)=64x64 */
    ae_spatial_init(&s, 0, 0, 1000, 1000, 5, 4, 4);

    /* A large entity whose AABB covers much of the world */
    ae_entity_t big   = ae_entity_make(100, 0);
    ae_aabb2_t  big_box = {0, 0, 600, 600};

    /* A small point-ish entity */
    ae_entity_t small_e = ae_entity_make(200, 0);
    ae_aabb2_t  small_box = {300, 300, 302, 302};

    ae_spatial_insert(&s, big,   &big_box);
    ae_spatial_insert(&s, small_e, &small_box);

    ae_spatial_result_t res;
    ae_spatial_result_init(&res);

    /* Query that overlaps both */
    ae_aabb2_t q = {290, 290, 310, 310};
    ae_spatial_query(&s, &q, &res);
    CHECK(result_contains(&res, big),   "big entity found");
    CHECK(result_contains(&res, small_e), "small entity found");

    ae_spatial_result_free(&res);
    ae_spatial_destroy(&s);
    printf("[spatial] multi-level done\n\n");
}

int main(void) {
    printf("=== Spatial Tests ===\n\n");

    test_init_destroy();
    test_insert_query_basic();
    test_remove();
    test_circle_query();
    test_multi_level();

    printf("=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed ? 1 : 0;
}
