/**
 * test_event.c — Unit tests for the lock-free event bus
 */
#include "ae_event.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>

static int passed = 0;
static int failed = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { printf("  PASS: %s\n", msg); ++passed; } \
        else      { printf("  FAIL: %s  (line %d)\n", msg, __LINE__); ++failed; } \
    } while (0)

/* ---- test event types --------------------------------------------------- */
#define EVT_MOVE  0
#define EVT_SPAWN 1
#define EVT_DIE   2

/* =========================================================================
 * Basic tests
 * ========================================================================= */

static void test_init_destroy(void) {
    printf("[event] init/destroy\n");
    ae_event_bus_t bus;
    CHECK(ae_bus_init(&bus, 1024) == AE_OK, "init ok");
    CHECK(ae_bus_init(&bus, 3) == AE_ERR_INVALID, "non-power-of-2 rejected");
    ae_bus_destroy(&bus);
    printf("[event] init/destroy done\n\n");
}

static atomic_int g_received;

static void handler_count(const ae_event_t *ev, void *ud) {
    (void)ev; (void)ud;
    atomic_fetch_add_explicit(&g_received, 1, memory_order_relaxed);
}

static void test_publish_drain(void) {
    printf("[event] publish + drain\n");
    ae_event_bus_t bus;
    ae_bus_init(&bus, 256);
    atomic_store(&g_received, 0);

    ae_bus_subscribe(&bus, EVT_MOVE,  handler_count, NULL);
    ae_bus_subscribe(&bus, EVT_SPAWN, handler_count, NULL);

    ae_event_t ev1 = { .type = EVT_MOVE,  .source = 1, .target = 2 };
    ae_event_t ev2 = { .type = EVT_SPAWN, .source = 3, .target = 0 };
    ae_event_t ev3 = { .type = EVT_DIE,   .source = 4, .target = 0 }; /* no sub */

    ae_bus_publish(&bus, &ev1);
    ae_bus_publish(&bus, &ev2);
    ae_bus_publish(&bus, &ev3);

    CHECK(ae_bus_pending(&bus) == 3, "3 events pending");

    uint64_t n = ae_bus_drain(&bus, 100);
    CHECK(n == 3, "drained 3 events");

    /* EVT_MOVE and EVT_SPAWN each got one call; EVT_DIE had no subscriber */
    CHECK(atomic_load(&g_received) == 2, "handler called 2 times");

    ae_bus_destroy(&bus);
    printf("[event] publish + drain done\n\n");
}

static void test_subscribe_unsubscribe(void) {
    printf("[event] subscribe/unsubscribe\n");
    ae_event_bus_t bus;
    ae_bus_init(&bus, 256);
    atomic_store(&g_received, 0);

    ae_bus_subscribe(&bus, EVT_MOVE, handler_count, NULL);

    ae_event_t ev = { .type = EVT_MOVE };
    ae_bus_publish(&bus, &ev);
    ae_bus_drain(&bus, 10);
    CHECK(atomic_load(&g_received) == 1, "handler called once before unsub");

    CHECK(ae_bus_unsubscribe(&bus, EVT_MOVE, handler_count, NULL) == AE_OK,
          "unsubscribe ok");

    ae_bus_publish(&bus, &ev);
    ae_bus_drain(&bus, 10);
    CHECK(atomic_load(&g_received) == 1, "handler not called after unsub");

    /* Unsubscribing again should return NOT_FOUND */
    CHECK(ae_bus_unsubscribe(&bus, EVT_MOVE, handler_count, NULL) == AE_ERR_NOT_FOUND,
          "double unsub => NOT_FOUND");

    ae_bus_destroy(&bus);
    printf("[event] subscribe/unsubscribe done\n\n");
}

static void test_ring_full(void) {
    printf("[event] ring buffer full\n");
    ae_event_bus_t bus;
    ae_bus_init(&bus, 4); /* tiny ring */

    ae_event_t ev = { .type = EVT_MOVE };
    ae_result_t r;

    /* Fill the ring */
    for (int i = 0; i < 4; ++i) {
        r = ae_bus_publish(&bus, &ev);
        CHECK(r == AE_OK, "publish to non-full ring");
    }

    /* Next publish must fail */
    r = ae_bus_publish(&bus, &ev);
    CHECK(r == AE_ERR_FULL, "publish to full ring => FULL");

    uint64_t dropped;
    ae_bus_stats(&bus, NULL, &dropped, NULL);
    CHECK(dropped >= 1, "dropped counter incremented");

    ae_bus_destroy(&bus);
    printf("[event] ring full done\n\n");
}

static void test_stats(void) {
    printf("[event] statistics\n");
    ae_event_bus_t bus;
    ae_bus_init(&bus, 64);

    ae_event_t ev = { .type = EVT_MOVE };
    for (int i = 0; i < 10; ++i) ae_bus_publish(&bus, &ev);

    uint64_t pub, drp, disp;
    ae_bus_stats(&bus, &pub, &drp, &disp);
    CHECK(pub == 10, "published == 10");
    CHECK(drp ==  0, "dropped == 0");

    ae_bus_drain(&bus, 100);
    ae_bus_stats(&bus, NULL, NULL, &disp);
    CHECK(disp == 10, "dispatched == 10");

    ae_bus_destroy(&bus);
    printf("[event] statistics done\n\n");
}

/* =========================================================================
 * Concurrent publish test (many producers, single consumer)
 * ========================================================================= */

#define NTHREADS    8
#define EVENTS_PER_THREAD 10000

static ae_event_bus_t g_bus;

static void *producer_thread(void *arg) {
    int id = (int)(intptr_t)arg;
    ae_event_t ev = { .type = EVT_MOVE, .source = (ae_entity_t)id };
    for (int i = 0; i < EVENTS_PER_THREAD; ++i) {
        /* Spin until published successfully */
        while (ae_bus_publish(&g_bus, &ev) != AE_OK)
            ; /* ring full — yield / retry */
    }
    return NULL;
}

static void test_concurrent_publish(void) {
    printf("[event] concurrent publish (%d threads × %d events)\n",
           NTHREADS, EVENTS_PER_THREAD);

    /* Large ring so almost no spin */
    ae_bus_init(&g_bus, 1u << 17); /* 131 072 slots */
    atomic_store(&g_received, 0);
    ae_bus_subscribe(&g_bus, EVT_MOVE, handler_count, NULL);

    pthread_t threads[NTHREADS];
    for (int i = 0; i < NTHREADS; ++i)
        pthread_create(&threads[i], NULL, producer_thread, (void *)(intptr_t)i);

    uint64_t total = (uint64_t)NTHREADS * EVENTS_PER_THREAD;
    uint64_t drained = 0;
    while (drained < total) {
        drained += ae_bus_drain(&g_bus, 4096);
    }

    for (int i = 0; i < NTHREADS; ++i)
        pthread_join(threads[i], NULL);

    CHECK((uint64_t)atomic_load(&g_received) == total,
          "all events dispatched to handler");

    uint64_t pub, drp;
    ae_bus_stats(&g_bus, &pub, &drp, NULL);
    CHECK(pub == total, "published counter matches");
    CHECK(drp ==     0, "no drops with large ring");

    ae_bus_destroy(&g_bus);
    printf("[event] concurrent publish done\n\n");
}

/* =========================================================================
 * Main
 * ========================================================================= */

int main(void) {
    printf("=== Event Bus Tests ===\n\n");

    test_init_destroy();
    test_publish_drain();
    test_subscribe_unsubscribe();
    test_ring_full();
    test_stats();
    test_concurrent_publish();

    printf("=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed ? 1 : 0;
}
