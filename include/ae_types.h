/**
 * ae_types.h — Common type definitions for aetoer
 *
 * A high-performance spatiotemporal event-driven engine written in pure C.
 * Targets 800M+ events/sec on a single machine with zero GC and full memory
 * transparency.
 */
#ifndef AETOER_TYPES_H
#define AETOER_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

/* ---- build configuration ------------------------------------------------ */
#ifndef AE_MAX_COMPONENTS
#  define AE_MAX_COMPONENTS  256
#endif

#ifndef AE_MAX_EVENT_TYPES
#  define AE_MAX_EVENT_TYPES 1024
#endif

#ifndef AE_SPATIAL_MAX_LEVELS
#  define AE_SPATIAL_MAX_LEVELS 6
#endif

/* ---- cache-line helpers -------------------------------------------------- */
#define AE_CACHE_LINE 64
#define AE_ALIGNED(n) __attribute__((aligned(n)))
#define AE_CL_ALIGNED AE_ALIGNED(AE_CACHE_LINE)

/* ---- entity ID ----------------------------------------------------------- */
/**
 * Entity ID packs a 32-bit generation counter into the high bits and a 32-bit
 * index into the low bits.  Generation prevents stale handles from silently
 * aliasing recycled entities (ABA prevention).
 */
typedef uint64_t ae_entity_t;

#define AE_ENTITY_INDEX_BITS  32u
#define AE_ENTITY_GEN_BITS    32u
#define AE_ENTITY_INDEX_MASK  UINT64_C(0x00000000FFFFFFFF)
#define AE_ENTITY_GEN_MASK    UINT64_C(0xFFFFFFFF00000000)
#define AE_ENTITY_INVALID     UINT64_MAX

static inline uint32_t ae_entity_index(ae_entity_t e) {
    return (uint32_t)(e & AE_ENTITY_INDEX_MASK);
}
static inline uint32_t ae_entity_gen(ae_entity_t e) {
    return (uint32_t)((e & AE_ENTITY_GEN_MASK) >> AE_ENTITY_INDEX_BITS);
}
static inline ae_entity_t ae_entity_make(uint32_t index, uint32_t gen) {
    return ((ae_entity_t)gen << AE_ENTITY_INDEX_BITS) | (ae_entity_t)index;
}

/* ---- component type ID --------------------------------------------------- */
typedef uint16_t ae_comp_id_t;
#define AE_COMP_INVALID ((ae_comp_id_t)0xFFFF)

/* ---- event type ID ------------------------------------------------------- */
typedef uint32_t ae_event_type_t;
#define AE_EVENT_INVALID ((ae_event_type_t)0xFFFFFFFF)

/* ---- 2D / 3D coordinates ------------------------------------------------- */
typedef struct { double x, y;    } ae_vec2_t;
typedef struct { double x, y, z; } ae_vec3_t;

typedef struct {
    double min_x, min_y;
    double max_x, max_y;
} ae_aabb2_t;

/* ---- return codes -------------------------------------------------------- */
typedef enum {
    AE_OK            =  0,
    AE_ERR_NOMEM     = -1,
    AE_ERR_FULL      = -2,
    AE_ERR_NOT_FOUND = -3,
    AE_ERR_INVALID   = -4,
    AE_ERR_DUPLICATE = -5,
} ae_result_t;

#endif /* AETOER_TYPES_H */
