/*
 * test_perf.c — Performance benchmarks for aetoer
 *
 * Needs _GNU_SOURCE for CLOCK_MONOTONIC_RAW on Linux.
 *
 * Measures throughput of:
 *  1. ECS component iteration
 *  2. Spatial grid insert + query
 *  3. Event bus publish+drain throughput (single-threaded)
 *  4. Event bus publish+drain throughput (multi-threaded)
 *
 * Numbers are printed as M ops/s or M events/s.
 */
#define _GNU_SOURCE
#include "aetoer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

/* =========================================================================
 * Timing
 * ========================================================================= */
static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* =========================================================================
 * 1. ECS iteration benchmark
 * ========================================================================= */

typedef struct { float x, y, z; }    PerfPos;
typedef struct { float vx, vy, vz; } PerfVel;

static void bench_ecs(void) {
    printf("[bench] ECS component iteration\n");

    const uint32_t N = 1000000; /* 1 M entities */
    ae_world_t *w = ae_world_create(N + 1, 64);
    ae_comp_id_t cpos = ae_comp_register(w, sizeof(PerfPos), "Pos");
    ae_comp_id_t cvel = ae_comp_register(w, sizeof(PerfVel), "Vel");

    /* Create N entities, all with Position; half with Velocity */
    PerfPos p = {1.0f, 2.0f, 3.0f};
    PerfVel v = {0.1f, 0.2f, 0.3f};

    for (uint32_t i = 0; i < N; ++i) {
        ae_entity_t e = ae_entity_create(w);
        ae_comp_add(w, e, cpos, &p);
        if (i % 2 == 0)
            ae_comp_add(w, e, cvel, &v);
    }

    /* Query & iterate: pos + vel */
    ae_comp_mask_t req = {{0}};
    ae_mask_set(&req, cpos);
    ae_mask_set(&req, cvel);

    const int ITERS = 10;
    double t0 = now_sec();
    volatile float sum = 0.0f; /* prevent DCE */

    for (int it = 0; it < ITERS; ++it) {
        ae_query_t q;
        ae_query_begin(&q, w, &req);
        ae_entity_t e;
        while ((e = ae_query_next(&q)) != AE_ENTITY_INVALID) {
            PerfPos *pp = (PerfPos *)ae_comp_get(w, e, cpos);
            PerfVel *pv = (PerfVel *)ae_comp_get(w, e, cvel);
            pp->x += pv->vx;
            sum += pp->x;
        }
    }

    double dt = now_sec() - t0;
    double total_ops = (double)(N / 2) * ITERS;
    printf("  %.0f M entity-updates/s  (sum=%.1f, prevent-dce)\n",
           total_ops / dt / 1e6, (double)sum);

    ae_world_destroy(w);
    printf("[bench] ECS done\n\n");
}

/* =========================================================================
 * 2. Spatial grid benchmark
 * ========================================================================= */

static void bench_spatial(void) {
    printf("[bench] Spatial grid insert + query\n");

    ae_spatial_t s;
    ae_spatial_init(&s, 0, 0, 10000, 10000, 5, 8, 8);

    const uint32_t N = 100000;
    ae_aabb2_t *boxes = (ae_aabb2_t *)malloc(N * sizeof(ae_aabb2_t));

    /* Random-ish positions */
    for (uint32_t i = 0; i < N; ++i) {
        double x = (double)(i % 10000);
        double y = (double)((i * 7) % 10000);
        boxes[i] = (ae_aabb2_t){x, y, x + 1.0, y + 1.0};
    }

    double t0 = now_sec();
    for (uint32_t i = 0; i < N; ++i)
        ae_spatial_insert(&s, ae_entity_make(i, 0), &boxes[i]);
    double t_ins = now_sec() - t0;
    printf("  Insert: %.1f M inserts/s\n", (double)N / t_ins / 1e6);

    /* Query: many small windows */
    ae_spatial_result_t res;
    ae_spatial_result_init(&res);

    const int QITERS = 10000;
    t0 = now_sec();
    for (int i = 0; i < QITERS; ++i) {
        ae_spatial_result_clear(&res);
        double cx = (double)((i * 137) % 9000);
        double cy = (double)((i * 251) % 9000);
        ae_aabb2_t q = {cx, cy, cx + 200, cy + 200};
        ae_spatial_query(&s, &q, &res);
    }
    double t_qry = now_sec() - t0;
    printf("  Query : %.1f K queries/s  (last result: %u entities)\n",
           (double)QITERS / t_qry / 1e3, res.count);

    ae_spatial_result_free(&res);
    free(boxes);
    ae_spatial_destroy(&s);
    printf("[bench] Spatial done\n\n");
}

/* =========================================================================
 * 3. Event bus single-threaded throughput
 * ========================================================================= */

static atomic_uint_fast64_t g_sink;

static void sink_handler(const ae_event_t *ev, void *ud) {
    (void)ud;
    /* Minimal work to avoid measuring just memcpy */
    atomic_fetch_add_explicit(&g_sink, ev->source, memory_order_relaxed);
}

static void bench_event_single(void) {
    printf("[bench] Event bus single-threaded\n");

    ae_event_bus_t bus;
    ae_bus_init(&bus, 1u << 20); /* 1 M slots */
    ae_bus_subscribe(&bus, 0, sink_handler, NULL);
    atomic_store(&g_sink, 0);

    const uint64_t N = 10000000ULL; /* 10 M events */
    ae_event_t ev = { .type = 0, .source = 1, .target = 2 };

    double t0 = now_sec();
    uint64_t published = 0, drained = 0;

    while (drained < N) {
        /* Batch-publish until ring is full */
        while (published < N && ae_bus_publish(&bus, &ev) == AE_OK) {
            ++published; ev.source = (ae_entity_t)(published & 0xFFFF);
        }
        drained += ae_bus_drain(&bus, 65536);
    }

    double dt = now_sec() - t0;
    printf("  %.1f M events/s  (single-thread publish+drain)\n",
           (double)N / dt / 1e6);

    ae_bus_destroy(&bus);
    printf("[bench] Event single-thread done\n\n");
}

/* =========================================================================
 * 4. Event bus multi-threaded throughput
 * ========================================================================= */

#define BENCH_THREADS 8

static ae_event_bus_t g_perf_bus;
static atomic_uint_fast64_t g_produced;
static const uint64_t TOTAL_EVENTS = 8000000ULL; /* 8 M */

static void *perf_producer(void *arg) {
    (void)arg;
    ae_event_t ev = { .type = 0, .source = 1 };
    for (;;) {
        uint64_t cnt = atomic_fetch_add_explicit(&g_produced, 1,
                                                  memory_order_relaxed);
        if (cnt >= TOTAL_EVENTS) break;
        while (ae_bus_publish(&g_perf_bus, &ev) != AE_OK)
            ; /* spin on full ring */
    }
    return NULL;
}

static void bench_event_multi(void) {
    printf("[bench] Event bus multi-threaded (%d producers)\n", BENCH_THREADS);

    ae_bus_init(&g_perf_bus, 1u << 20);
    ae_bus_subscribe(&g_perf_bus, 0, sink_handler, NULL);
    atomic_store(&g_produced, 0);
    atomic_store(&g_sink, 0);

    pthread_t threads[BENCH_THREADS];
    double t0 = now_sec();

    for (int i = 0; i < BENCH_THREADS; ++i)
        pthread_create(&threads[i], NULL, perf_producer, NULL);

    /* Consumer loop */
    uint64_t drained = 0;
    while (drained < TOTAL_EVENTS)
        drained += ae_bus_drain(&g_perf_bus, 65536);

    double dt = now_sec() - t0;

    for (int i = 0; i < BENCH_THREADS; ++i)
        pthread_join(threads[i], NULL);

    printf("  %.1f M events/s  (%d threads → 1 consumer)\n",
           (double)TOTAL_EVENTS / dt / 1e6, BENCH_THREADS);

    ae_bus_destroy(&g_perf_bus);
    printf("[bench] Event multi-thread done\n\n");
}

/* =========================================================================
 * Main
 * ========================================================================= */

int main(void) {
    printf("=== aetoer Performance Benchmarks ===\n\n");
    bench_ecs();
    bench_spatial();
    bench_event_single();
    bench_event_multi();
    printf("=== Benchmark complete ===\n");
    return 0;
}
