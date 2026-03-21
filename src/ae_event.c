/**
 * ae_event.c — Lock-free MPMC event bus implementation
 *
 * Ring buffer algorithm: Dmitry Vyukov's MPMC bounded queue.
 * Each slot carries a monotonic sequence counter.  Producers claim a slot by
 * CAS-incrementing the tail; consumers claim by CAS-incrementing the head.
 * A slot is ready to write when sequence == position; ready to read when
 * sequence == position + 1.
 */
#include "ae_event.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

/* =========================================================================
 * Helpers
 * ========================================================================= */

static inline bool is_power_of_two(size_t n) {
    return n != 0 && (n & (n - 1)) == 0;
}

static uint64_t now_ns(void) {
#if defined(_POSIX_MONOTONIC_CLOCK)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
#else
    return 0;
#endif
}

/* =========================================================================
 * Init / Destroy
 * ========================================================================= */

ae_result_t ae_bus_init(ae_event_bus_t *bus, size_t capacity) {
    if (!bus) return AE_ERR_INVALID;
    if (!is_power_of_two(capacity) || capacity < 2) return AE_ERR_INVALID;

    memset(bus, 0, sizeof(*bus));

    bus->slots = (ae_ring_slot_t *)aligned_alloc(
                     AE_CACHE_LINE,
                     capacity * sizeof(ae_ring_slot_t));
    if (!bus->slots) return AE_ERR_NOMEM;

    bus->capacity = capacity;
    bus->mask     = capacity - 1;

    /* Initialise each slot's sequence to its position */
    for (size_t i = 0; i < capacity; ++i)
        atomic_store_explicit(&bus->slots[i].sequence, i,
                              memory_order_relaxed);

    atomic_store_explicit(&bus->head, 0, memory_order_relaxed);
    atomic_store_explicit(&bus->tail, 0, memory_order_relaxed);
    atomic_store_explicit(&bus->published,  0, memory_order_relaxed);
    atomic_store_explicit(&bus->dropped,    0, memory_order_relaxed);
    atomic_store_explicit(&bus->dispatched, 0, memory_order_relaxed);

    return AE_OK;
}

void ae_bus_destroy(ae_event_bus_t *bus) {
    if (!bus) return;
    free(bus->slots);
    bus->slots = NULL;
}

/* =========================================================================
 * Publish (MPMC producer side)
 * ========================================================================= */

ae_result_t ae_bus_publish(ae_event_bus_t *bus, const ae_event_t *event) {
    if (!bus || !event) return AE_ERR_INVALID;

    ae_ring_slot_t *slot;
    size_t pos = atomic_load_explicit(&bus->tail, memory_order_relaxed);

    for (;;) {
        slot = &bus->slots[pos & bus->mask];
        size_t seq = atomic_load_explicit(&slot->sequence,
                                           memory_order_acquire);
        intptr_t diff = (intptr_t)seq - (intptr_t)pos;

        if (diff == 0) {
            /* Slot is free; try to claim it */
            if (atomic_compare_exchange_weak_explicit(
                        &bus->tail, &pos, pos + 1,
                        memory_order_relaxed, memory_order_relaxed))
                break;
            /* CAS failed — retry */
        } else if (diff < 0) {
            /* Ring buffer full */
            atomic_fetch_add_explicit(&bus->dropped, 1, memory_order_relaxed);
            return AE_ERR_FULL;
        } else {
            /* Another producer advanced tail; re-read */
            pos = atomic_load_explicit(&bus->tail, memory_order_relaxed);
        }
    }

    slot->event = *event;
    /* Mark slot ready for consumer */
    atomic_store_explicit(&slot->sequence, pos + 1, memory_order_release);
    atomic_fetch_add_explicit(&bus->published, 1, memory_order_relaxed);
    return AE_OK;
}

ae_result_t ae_bus_publish_now(ae_event_bus_t *bus,
                                ae_event_type_t type,
                                ae_entity_t source,
                                ae_entity_t target,
                                const void *payload, size_t payload_len) {
    ae_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type         = type;
    ev.source       = source;
    ev.target       = target;
    ev.timestamp_ns = now_ns();
    if (payload && payload_len > 0) {
        size_t copy = payload_len < AE_EVENT_PAYLOAD_SIZE
                      ? payload_len : AE_EVENT_PAYLOAD_SIZE;
        memcpy(ev.payload, payload, copy);
    }
    return ae_bus_publish(bus, &ev);
}

/* =========================================================================
 * Drain (single consumer side)
 * ========================================================================= */

uint64_t ae_bus_drain(ae_event_bus_t *bus, uint64_t max_events) {
    if (!bus) return 0;

    uint64_t processed = 0;
    size_t   pos = atomic_load_explicit(&bus->head, memory_order_relaxed);

    while (processed < max_events) {
        ae_ring_slot_t *slot = &bus->slots[pos & bus->mask];
        size_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);
        intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);

        if (diff < 0) {
            /* No event available yet */
            break;
        } else if (diff == 0) {
            /* Slot is ready; try to consume */
            if (!atomic_compare_exchange_weak_explicit(
                        &bus->head, &pos, pos + 1,
                        memory_order_relaxed, memory_order_relaxed))
                continue;

            /* Dispatch to subscribers */
            ae_event_type_t type = slot->event.type;
            if (type < AE_MAX_EVENT_TYPES) {
                uint32_t cnt = bus->sub_cnt[type];
                for (uint32_t i = 0; i < cnt; ++i) {
                    ae_subscriber_t *sub = &bus->subs[type][i];
                    if (sub->fn) sub->fn(&slot->event, sub->ud);
                }
            }

            /* Release slot for reuse */
            atomic_store_explicit(&slot->sequence,
                                   pos + bus->capacity,
                                   memory_order_release);
            ++processed;
            ++pos;
            atomic_fetch_add_explicit(&bus->dispatched, 1,
                                      memory_order_relaxed);
        } else {
            /* Another consumer advanced head; re-read */
            pos = atomic_load_explicit(&bus->head, memory_order_relaxed);
        }
    }

    return processed;
}

/* =========================================================================
 * Subscribe / Unsubscribe
 * ========================================================================= */

ae_result_t ae_bus_subscribe(ae_event_bus_t *bus,
                              ae_event_type_t type,
                              ae_event_handler_t fn,
                              void *userdata) {
    if (!bus || !fn)            return AE_ERR_INVALID;
    if (type >= AE_MAX_EVENT_TYPES) return AE_ERR_INVALID;

    uint32_t cnt = bus->sub_cnt[type];
    if (cnt >= AE_MAX_SUBSCRIBERS) return AE_ERR_FULL;

    bus->subs[type][cnt].fn = fn;
    bus->subs[type][cnt].ud = userdata;
    bus->sub_cnt[type]      = cnt + 1;
    return AE_OK;
}

ae_result_t ae_bus_unsubscribe(ae_event_bus_t *bus,
                                ae_event_type_t type,
                                ae_event_handler_t fn,
                                void *userdata) {
    if (!bus || !fn)            return AE_ERR_INVALID;
    if (type >= AE_MAX_EVENT_TYPES) return AE_ERR_INVALID;

    uint32_t cnt = bus->sub_cnt[type];
    for (uint32_t i = 0; i < cnt; ++i) {
        if (bus->subs[type][i].fn == fn &&
            bus->subs[type][i].ud == userdata) {
            /* Swap with last */
            bus->subs[type][i] = bus->subs[type][cnt - 1];
            bus->sub_cnt[type] = cnt - 1;
            return AE_OK;
        }
    }
    return AE_ERR_NOT_FOUND;
}

/* =========================================================================
 * Stats
 * ========================================================================= */

size_t ae_bus_pending(const ae_event_bus_t *bus) {
    size_t tail = atomic_load_explicit(&bus->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&bus->head, memory_order_relaxed);
    return tail >= head ? tail - head : 0;
}

void ae_bus_stats(const ae_event_bus_t *bus,
                  uint64_t *published, uint64_t *dropped,
                  uint64_t *dispatched) {
    if (!bus) return;
    if (published)
        *published  = atomic_load_explicit(&bus->published,
                                            memory_order_relaxed);
    if (dropped)
        *dropped    = atomic_load_explicit(&bus->dropped,
                                            memory_order_relaxed);
    if (dispatched)
        *dispatched = atomic_load_explicit(&bus->dispatched,
                                            memory_order_relaxed);
}
