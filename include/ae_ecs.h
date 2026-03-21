/**
 * ae_ecs.h — Archetype-less ECS (Entity-Component-System) for aetoer
 *
 * Design: each component type is stored in its own sparse set, giving O(1)
 * add / remove / lookup while keeping component data packed for cache-friendly
 * iteration.  Entities are recycled handles combining a 32-bit index and a
 * 32-bit generation counter (ABA-safe).
 *
 * Thread model: single-writer / multiple-reader per world.  Concurrent reads
 * of component data are safe; mutations must be serialised by the caller.
 */
#ifndef AETOER_ECS_H
#define AETOER_ECS_H

#include "ae_types.h"
#include "ae_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Component mask (bitset over AE_MAX_COMPONENTS component IDs)
 * ========================================================================= */
#define AE_COMP_MASK_WORDS ((AE_MAX_COMPONENTS + 63) / 64)

typedef struct {
    uint64_t bits[AE_COMP_MASK_WORDS];
} ae_comp_mask_t;

static inline void ae_mask_set(ae_comp_mask_t *m, ae_comp_id_t id) {
    m->bits[id >> 6] |= UINT64_C(1) << (id & 63);
}
static inline void ae_mask_clear(ae_comp_mask_t *m, ae_comp_id_t id) {
    m->bits[id >> 6] &= ~(UINT64_C(1) << (id & 63));
}
static inline bool ae_mask_has(const ae_comp_mask_t *m, ae_comp_id_t id) {
    return (m->bits[id >> 6] >> (id & 63)) & 1;
}
static inline bool ae_mask_contains(const ae_comp_mask_t *a,
                                    const ae_comp_mask_t *b) {
    /* returns true when every bit set in b is also set in a */
    for (int i = 0; i < AE_COMP_MASK_WORDS; ++i)
        if ((a->bits[i] & b->bits[i]) != b->bits[i]) return false;
    return true;
}

/* =========================================================================
 * Sparse set  (per component type)
 * ========================================================================= */
#define AE_SPARSE_INVALID UINT32_MAX

typedef struct {
    uint32_t *sparse;   /* sparse[entity_index] → dense index or INVALID     */
    uint32_t *dense;    /* dense[dense_idx]    → entity index                 */
    uint32_t *gen;      /* gen[dense_idx]      → entity generation            */
    uint8_t  *data;     /* packed component data (comp_size bytes per entry)  */
    uint32_t  count;    /* number of live entries                             */
    uint32_t  sparse_cap; /* capacity of sparse array (max entity index + 1) */
    uint32_t  dense_cap;  /* capacity of dense / data arrays                  */
    size_t    comp_size;  /* sizeof one component                             */
} ae_sparse_set_t;

/* =========================================================================
 * World
 * ========================================================================= */

typedef struct ae_world ae_world_t;

/**
 * Create a world that can hold up to @max_entities entities and
 * @max_comp_types distinct component types.
 */
ae_world_t *ae_world_create(uint32_t max_entities, uint16_t max_comp_types);

/** Destroy the world and all associated memory. */
void ae_world_destroy(ae_world_t *w);

/* ---- entity lifecycle --------------------------------------------------- */

/** Create a new entity; returns AE_ENTITY_INVALID on OOM. */
ae_entity_t ae_entity_create(ae_world_t *w);

/**
 * Destroy an entity and remove all its components.
 * The ID is recycled; the generation is incremented to invalidate old handles.
 */
void ae_entity_destroy(ae_world_t *w, ae_entity_t e);

/** Returns true when the entity is alive in this world. */
bool ae_entity_alive(const ae_world_t *w, ae_entity_t e);

/* ---- component registration --------------------------------------------- */

/**
 * Register a component type with the world and return its ID.
 * @comp_size   : sizeof(YourComponent)
 * @name        : human-readable name (for debugging; may be NULL)
 * Returns AE_COMP_INVALID when too many component types are registered.
 */
ae_comp_id_t ae_comp_register(ae_world_t *w, size_t comp_size,
                               const char *name);

/* ---- component operations ----------------------------------------------- */

/**
 * Add a component to an entity, copying @data (comp_size bytes) into the
 * sparse set.  If the component already exists it is replaced.
 * Returns AE_OK or AE_ERR_NOMEM.
 */
ae_result_t ae_comp_add(ae_world_t *w, ae_entity_t e, ae_comp_id_t cid,
                         const void *data);

/**
 * Remove a component from an entity.
 * Returns AE_ERR_NOT_FOUND if the entity did not have this component.
 */
ae_result_t ae_comp_remove(ae_world_t *w, ae_entity_t e, ae_comp_id_t cid);

/**
 * Get a pointer to a component (or NULL if the entity doesn't have it).
 * The pointer is valid until the next add/remove on the same component type.
 */
void *ae_comp_get(const ae_world_t *w, ae_entity_t e, ae_comp_id_t cid);

/** Returns true when the entity has the given component. */
bool ae_comp_has(const ae_world_t *w, ae_entity_t e, ae_comp_id_t cid);

/* ---- query / iteration -------------------------------------------------- */

/**
 * Opaque query cursor.  Iterate over all entities that have ALL components
 * specified in the required mask.
 */
typedef struct {
    ae_world_t          *world;
    ae_comp_mask_t       required;
    ae_comp_id_t         pivot; /* component type with fewest entities */
    uint32_t             idx;   /* current position in pivot's dense array */
} ae_query_t;

/**
 * Begin a query.  @required is a mask built with ae_mask_set().
 * Finds the sparsest component to use as the iteration pivot automatically.
 */
void ae_query_begin(ae_query_t *q, ae_world_t *w,
                    const ae_comp_mask_t *required);

/**
 * Advance to the next matching entity.
 * Returns AE_ENTITY_INVALID when iteration is finished.
 */
ae_entity_t ae_query_next(ae_query_t *q);

/* ---- component name lookup (debug) -------------------------------------- */
const char *ae_comp_name(const ae_world_t *w, ae_comp_id_t cid);

/** Return the number of live entities in this world. */
uint32_t ae_world_entity_count(const ae_world_t *w);

#ifdef __cplusplus
}
#endif

#endif /* AETOER_ECS_H */
