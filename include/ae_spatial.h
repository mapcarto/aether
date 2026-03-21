/**
 * ae_spatial.h — Pyramid spatial grid index for aetoer
 *
 * A multi-level regular grid (pyramid) for 2-D space.  Each level halves the
 * cell size relative to the next coarser level, giving O(log N) query
 * complexity while keeping memory layout flat and cache-friendly.
 *
 * Entities are placed at the finest level whose cell still fully contains the
 * entity's AABB.  Point entities (zero-size) always land at the finest level.
 *
 * Thread model: insertions / removals must be serialised.  Concurrent
 * read-only queries (ae_spatial_query_*) are safe.
 */
#ifndef AETOER_SPATIAL_H
#define AETOER_SPATIAL_H

#include "ae_types.h"
#include "ae_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Cell entry (linked list node inside each grid cell)
 * ========================================================================= */
typedef struct ae_cell_node {
    ae_entity_t          entity;
    ae_aabb2_t           aabb;
    struct ae_cell_node *next;
} ae_cell_node_t;

/* =========================================================================
 * Single grid level
 * ========================================================================= */
typedef struct {
    ae_cell_node_t **cells;    /* flat array of cell-list heads [rows*cols] */
    uint32_t         rows;
    uint32_t         cols;
    double           cell_w;   /* world-space width  of one cell */
    double           cell_h;   /* world-space height of one cell */
} ae_grid_level_t;

/* =========================================================================
 * Pyramid spatial index
 * ========================================================================= */
typedef struct {
    ae_grid_level_t levels[AE_SPATIAL_MAX_LEVELS];
    uint32_t        num_levels;
    double          world_min_x, world_min_y;
    double          world_max_x, world_max_y;
    ae_pool_t       node_pool; /* pool for ae_cell_node_t objects */
    uint64_t        entity_count;
} ae_spatial_t;

/* =========================================================================
 * Query result (dynamic array of entity IDs)
 * ========================================================================= */
typedef struct {
    ae_entity_t *entities;
    uint32_t     count;
    uint32_t     capacity;
} ae_spatial_result_t;

/* =========================================================================
 * API
 * ========================================================================= */

/**
 * Initialise the pyramid index.
 *
 * @s            : index to initialise
 * @world_min_x/y: world-space bounding box (lower-left)
 * @world_max_x/y: world-space bounding box (upper-right)
 * @num_levels   : number of pyramid levels (1–AE_SPATIAL_MAX_LEVELS)
 *                 Level 0 is the coarsest (num_levels-1 is finest).
 * @base_cols/rows: cell count along each axis at the coarsest level
 *                  Each subsequent level doubles the resolution.
 */
ae_result_t ae_spatial_init(ae_spatial_t *s,
                             double world_min_x, double world_min_y,
                             double world_max_x, double world_max_y,
                             uint32_t num_levels,
                             uint32_t base_cols, uint32_t base_rows);

/** Destroy the spatial index and release all memory. */
void ae_spatial_destroy(ae_spatial_t *s);

/**
 * Insert an entity with the given AABB.
 * For point entities set aabb.min == aabb.max.
 */
ae_result_t ae_spatial_insert(ae_spatial_t *s, ae_entity_t entity,
                               const ae_aabb2_t *aabb);

/**
 * Remove an entity from the index (searches all matching cells).
 * Returns AE_ERR_NOT_FOUND if the entity was not present.
 */
ae_result_t ae_spatial_remove(ae_spatial_t *s, ae_entity_t entity,
                               const ae_aabb2_t *aabb);

/**
 * Query all entities whose AABBs intersect @query_box.
 * Results are appended to @result (must be initialised by caller; use
 * ae_spatial_result_init / ae_spatial_result_free).
 * Duplicate entity IDs may appear when an entity spans multiple cells; the
 * caller should deduplicate if needed.
 */
ae_result_t ae_spatial_query(const ae_spatial_t *s,
                              const ae_aabb2_t   *query_box,
                              ae_spatial_result_t *result);

/**
 * Convenience: query using a circle (converted to AABB internally).
 */
ae_result_t ae_spatial_query_circle(const ae_spatial_t *s,
                                     double cx, double cy, double radius,
                                     ae_spatial_result_t *result);

/** Initialise a result container. */
void ae_spatial_result_init(ae_spatial_result_t *r);

/** Free a result container. */
void ae_spatial_result_free(ae_spatial_result_t *r);

/** Clear (reset) a result container for reuse without freeing memory. */
void ae_spatial_result_clear(ae_spatial_result_t *r);

#ifdef __cplusplus
}
#endif

#endif /* AETOER_SPATIAL_H */
