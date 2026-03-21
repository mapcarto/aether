/**
 * ae_event.h — Lock-free MPMC event bus for aetoer
 *
 * Architecture:
 *  - Single ring buffer (power-of-2 capacity) based on Dmitry Vyukov's
 *    "1024cores" MPMC queue.  Each slot carries a sequence counter that
 *    allows producers and consumers to operate without any mutex.
 *  - Subscribers register per event-type callbacks; dispatch fans out to
 *    all registered handlers for the matching type.
 *  - The bus is safe to publish to from multiple threads simultaneously.
 *    Subscriber callbacks are invoked from the draining thread(s).
 *
 * Throughput target: ≥ 800 M events/s on a modern 8-core machine.
 */
#ifndef AETOER_EVENT_H
#define AETOER_EVENT_H

#include "ae_types.h"
#include "ae_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Event payload
 * ========================================================================= */
#define AE_EVENT_PAYLOAD_SIZE 48

typedef struct {
    ae_event_type_t type;           /* event type ID                        */
    ae_entity_t     source;         /* originating entity (may be INVALID)  */
    ae_entity_t     target;         /* target entity     (may be INVALID)   */
    uint64_t        timestamp_ns;   /* nanosecond timestamp at publish time  */
    uint8_t         payload[AE_EVENT_PAYLOAD_SIZE]; /* user data            */
} ae_event_t;

/* Compile-time size assertion: keep ae_event_t a multiple of cache-line.   */
_Static_assert(sizeof(ae_event_t) == 80,
               "ae_event_t size changed — check padding");

/* =========================================================================
 * Ring-buffer slot
 * ========================================================================= */
typedef struct {
    AE_CL_ALIGNED atomic_size_t sequence; /* monotonic sequence counter     */
    ae_event_t                  event;
} ae_ring_slot_t;

/* =========================================================================
 * Bus
 * ========================================================================= */

/** Max subscribers per event type (compile-time). */
#ifndef AE_MAX_SUBSCRIBERS
#  define AE_MAX_SUBSCRIBERS 32
#endif

typedef void (*ae_event_handler_t)(const ae_event_t *event, void *userdata);

typedef struct {
    ae_event_handler_t fn;
    void              *ud;
} ae_subscriber_t;

typedef struct ae_event_bus {
    /* ring buffer */
    ae_ring_slot_t *slots;       /* heap-allocated ring slots              */
    size_t          capacity;    /* must be a power of 2                   */
    size_t          mask;        /* capacity - 1                           */

    AE_CL_ALIGNED atomic_size_t head; /* consumer position                */
    AE_CL_ALIGNED atomic_size_t tail; /* producer position                */

    /* subscriber table */
    ae_subscriber_t subs[AE_MAX_EVENT_TYPES][AE_MAX_SUBSCRIBERS];
    uint32_t        sub_cnt[AE_MAX_EVENT_TYPES];

    /* statistics */
    atomic_uint_fast64_t published;
    atomic_uint_fast64_t dropped;
    atomic_uint_fast64_t dispatched;
} ae_event_bus_t;

/* =========================================================================
 * API
 * ========================================================================= */

/**
 * Initialise an event bus.
 * @capacity must be a power of 2 (e.g. 1 << 20 for ~1 M slots).
 */
ae_result_t ae_bus_init(ae_event_bus_t *bus, size_t capacity);

/** Destroy the bus and release all memory. */
void ae_bus_destroy(ae_event_bus_t *bus);

/**
 * Publish an event to the ring buffer (non-blocking, lock-free).
 * May be called concurrently from multiple threads.
 * Returns AE_ERR_FULL if the ring buffer is full.
 */
ae_result_t ae_bus_publish(ae_event_bus_t *bus, const ae_event_t *event);

/**
 * Convenience: publish with a runtime-set nanosecond timestamp.
 * On Linux this calls clock_gettime(CLOCK_MONOTONIC_RAW).
 */
ae_result_t ae_bus_publish_now(ae_event_bus_t *bus,
                                ae_event_type_t type,
                                ae_entity_t source,
                                ae_entity_t target,
                                const void *payload, size_t payload_len);

/**
 * Drain up to @max_events events from the ring buffer, invoking the
 * registered subscriber callbacks for each.
 * Returns the number of events actually processed.
 * Safe to call from a single dedicated consumer thread.
 */
uint64_t ae_bus_drain(ae_event_bus_t *bus, uint64_t max_events);

/**
 * Register a subscriber callback for a given event type.
 * Returns AE_ERR_FULL if AE_MAX_SUBSCRIBERS is exhausted for that type.
 */
ae_result_t ae_bus_subscribe(ae_event_bus_t *bus,
                              ae_event_type_t type,
                              ae_event_handler_t fn,
                              void *userdata);

/**
 * Unregister a previously registered callback (matched by fn + userdata).
 * Returns AE_ERR_NOT_FOUND when the pair was not registered.
 */
ae_result_t ae_bus_unsubscribe(ae_event_bus_t *bus,
                                ae_event_type_t type,
                                ae_event_handler_t fn,
                                void *userdata);

/**
 * Returns approximate number of pending (unconsumed) events.
 * Not an exact count due to concurrent producers.
 */
size_t ae_bus_pending(const ae_event_bus_t *bus);

/** Read the cumulative published / dropped / dispatched counters. */
void ae_bus_stats(const ae_event_bus_t *bus,
                  uint64_t *published, uint64_t *dropped,
                  uint64_t *dispatched);

#ifdef __cplusplus
}
#endif

#endif /* AETOER_EVENT_H */
