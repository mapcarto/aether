/**
 * test_ecs.c — Unit tests for the archetype-less ECS
 */
#include "ae_ecs.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int passed = 0;
static int failed = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { printf("  PASS: %s\n", msg); ++passed; } \
        else      { printf("  FAIL: %s  (line %d)\n", msg, __LINE__); ++failed; } \
    } while (0)

/* ---- test component types ----------------------------------------------- */
typedef struct { float x, y, z; }    Position;
typedef struct { float vx, vy, vz; } Velocity;
typedef struct { int health; }       Health;

static void test_world_lifecycle(void) {
    printf("[ecs] world lifecycle\n");
    ae_world_t *w = ae_world_create(1024, 64);
    CHECK(w != NULL, "world created");
    CHECK(ae_world_entity_count(w) == 0, "empty world has 0 entities");
    ae_world_destroy(w);
    printf("[ecs] world lifecycle done\n\n");
}

static void test_entity_lifecycle(void) {
    printf("[ecs] entity lifecycle\n");
    ae_world_t *w = ae_world_create(1024, 64);

    ae_entity_t e1 = ae_entity_create(w);
    CHECK(e1 != AE_ENTITY_INVALID, "create entity 1");
    CHECK(ae_entity_alive(w, e1), "entity 1 alive");
    CHECK(ae_world_entity_count(w) == 1, "count == 1");

    ae_entity_t e2 = ae_entity_create(w);
    (void)e2;
    CHECK(ae_world_entity_count(w) == 2, "count == 2");

    ae_entity_destroy(w, e1);
    CHECK(!ae_entity_alive(w, e1), "entity 1 dead after destroy");
    CHECK(ae_world_entity_count(w) == 1, "count back to 1");

    /* After destroying e1, a new entity should recycle the slot but bump gen */
    ae_entity_t e3 = ae_entity_create(w);
    CHECK(e3 != AE_ENTITY_INVALID, "create after recycle");
    CHECK(ae_entity_index(e3) == ae_entity_index(e1), "recycled same index");
    CHECK(ae_entity_gen(e3) != ae_entity_gen(e1), "different generation");
    CHECK(!ae_entity_alive(w, e1), "stale handle still dead");

    ae_world_destroy(w);
    printf("[ecs] entity lifecycle done\n\n");
}

static void test_component_operations(void) {
    printf("[ecs] component operations\n");
    ae_world_t *w = ae_world_create(1024, 64);

    ae_comp_id_t cpos = ae_comp_register(w, sizeof(Position), "Position");
    ae_comp_id_t cvel = ae_comp_register(w, sizeof(Velocity), "Velocity");
    ae_comp_id_t chp  = ae_comp_register(w, sizeof(Health),   "Health");

    CHECK(cpos != AE_COMP_INVALID, "register Position");
    CHECK(cvel != AE_COMP_INVALID, "register Velocity");
    CHECK(chp  != AE_COMP_INVALID, "register Health");

    ae_entity_t e = ae_entity_create(w);

    Position pos = {1.0f, 2.0f, 3.0f};
    CHECK(ae_comp_add(w, e, cpos, &pos) == AE_OK, "add Position");
    CHECK(ae_comp_has(w, e, cpos), "has Position");
    CHECK(!ae_comp_has(w, e, cvel), "no Velocity yet");

    Position *pp = (Position *)ae_comp_get(w, e, cpos);
    CHECK(pp != NULL, "get Position ptr");
    CHECK(pp->x == 1.0f && pp->y == 2.0f && pp->z == 3.0f, "Position values correct");

    /* Update in-place */
    pp->x = 10.0f;
    Position *pp2 = (Position *)ae_comp_get(w, e, cpos);
    CHECK(pp2->x == 10.0f, "in-place update persists");

    /* Remove */
    CHECK(ae_comp_remove(w, e, cpos) == AE_OK, "remove Position");
    CHECK(!ae_comp_has(w, e, cpos), "Position removed");

    /* Double-remove returns error */
    CHECK(ae_comp_remove(w, e, cpos) == AE_ERR_NOT_FOUND, "double remove => NOT_FOUND");

    ae_world_destroy(w);
    printf("[ecs] component operations done\n\n");
}

static void test_query(void) {
    printf("[ecs] query\n");
    ae_world_t *w = ae_world_create(1024, 64);

    ae_comp_id_t cpos = ae_comp_register(w, sizeof(Position), "Position");
    ae_comp_id_t cvel = ae_comp_register(w, sizeof(Velocity), "Velocity");
    ae_comp_id_t chp  = ae_comp_register(w, sizeof(Health),   "Health");

    /* Create several entities with different component combinations */
    ae_entity_t e1 = ae_entity_create(w); /* pos + vel */
    ae_entity_t e2 = ae_entity_create(w); /* pos + vel + health */
    ae_entity_t e3 = ae_entity_create(w); /* pos only */
    ae_entity_t e4 = ae_entity_create(w); /* vel + health */

    Position  p = {0};
    Velocity  v = {0};
    Health    h = {100};

    ae_comp_add(w, e1, cpos, &p); ae_comp_add(w, e1, cvel, &v);
    ae_comp_add(w, e2, cpos, &p); ae_comp_add(w, e2, cvel, &v);
    ae_comp_add(w, e2, chp,  &h);
    ae_comp_add(w, e3, cpos, &p);
    ae_comp_add(w, e4, cvel, &v); ae_comp_add(w, e4, chp, &h);

    /* Query: entities with Position AND Velocity */
    ae_comp_mask_t req = {{0}};
    ae_mask_set(&req, cpos);
    ae_mask_set(&req, cvel);

    ae_query_t q;
    ae_query_begin(&q, w, &req);

    int count = 0;
    ae_entity_t e;
    while ((e = ae_query_next(&q)) != AE_ENTITY_INVALID) ++count;
    CHECK(count == 2, "pos+vel query returns 2 entities");

    /* Query: all three */
    ae_comp_mask_t req3 = {{0}};
    ae_mask_set(&req3, cpos);
    ae_mask_set(&req3, cvel);
    ae_mask_set(&req3, chp);

    ae_query_begin(&q, w, &req3);
    count = 0;
    while ((e = ae_query_next(&q)) != AE_ENTITY_INVALID) ++count;
    CHECK(count == 1, "pos+vel+health query returns 1 entity");

    ae_world_destroy(w);
    printf("[ecs] query done\n\n");
}

static void test_sparse_set_swap(void) {
    printf("[ecs] sparse-set swap on remove\n");
    ae_world_t *w = ae_world_create(1024, 64);
    ae_comp_id_t cpos = ae_comp_register(w, sizeof(Position), "Position");

    /* Create 5 entities, all with Position */
    ae_entity_t ents[5];
    for (int i = 0; i < 5; ++i) {
        ents[i] = ae_entity_create(w);
        Position p = {(float)i, (float)i, (float)i};
        ae_comp_add(w, ents[i], cpos, &p);
    }

    /* Remove the middle one */
    ae_comp_remove(w, ents[2], cpos);

    /* Remaining 4 should all still have their correct data */
    int ok = 1;
    for (int i = 0; i < 5; ++i) {
        if (i == 2) {
            if (ae_comp_has(w, ents[i], cpos)) { ok = 0; break; }
            continue;
        }
        Position *pp = (Position *)ae_comp_get(w, ents[i], cpos);
        if (!pp || pp->x != (float)i) { ok = 0; break; }
    }
    CHECK(ok, "remaining entities have correct data after sparse-set swap");

    ae_world_destroy(w);
    printf("[ecs] sparse-set swap done\n\n");
}

int main(void) {
    printf("=== ECS Tests ===\n\n");

    test_world_lifecycle();
    test_entity_lifecycle();
    test_component_operations();
    test_query();
    test_sparse_set_swap();

    printf("=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed ? 1 : 0;
}
