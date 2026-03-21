/**
 * ae_spatial.c — Pyramid spatial grid index implementation
 */
#include "ae_spatial.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

/* =========================================================================
 * Helpers
 * ========================================================================= */

static inline int32_t clamp_i(int32_t v, int32_t lo, int32_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* Convert world coordinate to cell column/row for a given level */
static void world_to_cell(const ae_spatial_t *s, int lvl,
                           double wx, double wy,
                           int32_t *col, int32_t *row) {
    const ae_grid_level_t *L = &s->levels[lvl];
    double rel_x = wx - s->world_min_x;
    double rel_y = wy - s->world_min_y;
    int32_t c = (int32_t)(rel_x / L->cell_w);
    int32_t r = (int32_t)(rel_y / L->cell_h);
    *col = clamp_i(c, 0, (int32_t)L->cols - 1);
    *row = clamp_i(r, 0, (int32_t)L->rows - 1);
}

static inline ae_cell_node_t **cell_at(const ae_grid_level_t *L,
                                        int32_t col, int32_t row) {
    return &L->cells[(size_t)row * L->cols + (size_t)col];
}

/* =========================================================================
 * Level allocation / deallocation
 * ========================================================================= */

static ae_result_t level_init(ae_grid_level_t *L,
                               uint32_t cols, uint32_t rows,
                               double cell_w, double cell_h) {
    L->cols   = cols;
    L->rows   = rows;
    L->cell_w = cell_w;
    L->cell_h = cell_h;
    size_t n  = (size_t)cols * rows;
    L->cells  = (ae_cell_node_t **)calloc(n, sizeof(ae_cell_node_t *));
    if (!L->cells) return AE_ERR_NOMEM;
    return AE_OK;
}

static void level_destroy(ae_grid_level_t *L) {
    free(L->cells);
    L->cells = NULL;
}

/* =========================================================================
 * Init / Destroy
 * ========================================================================= */

ae_result_t ae_spatial_init(ae_spatial_t *s,
                             double world_min_x, double world_min_y,
                             double world_max_x, double world_max_y,
                             uint32_t num_levels,
                             uint32_t base_cols, uint32_t base_rows) {
    if (!s) return AE_ERR_INVALID;
    if (num_levels < 1 || num_levels > AE_SPATIAL_MAX_LEVELS)
        return AE_ERR_INVALID;
    if (base_cols == 0 || base_rows == 0) return AE_ERR_INVALID;
    if (world_max_x <= world_min_x || world_max_y <= world_min_y)
        return AE_ERR_INVALID;

    memset(s, 0, sizeof(*s));
    s->world_min_x = world_min_x;
    s->world_min_y = world_min_y;
    s->world_max_x = world_max_x;
    s->world_max_y = world_max_y;
    s->num_levels  = num_levels;

    double world_w = world_max_x - world_min_x;
    double world_h = world_max_y - world_min_y;

    /* Level 0 is coarsest (base_cols x base_rows).
     * Each subsequent level doubles the resolution. */
    for (uint32_t lvl = 0; lvl < num_levels; ++lvl) {
        uint32_t cols    = base_cols  << lvl;
        uint32_t rows    = base_rows  << lvl;
        double   cell_w  = world_w / cols;
        double   cell_h  = world_h / rows;
        ae_result_t r    = level_init(&s->levels[lvl], cols, rows,
                                       cell_w, cell_h);
        if (r != AE_OK) {
            for (uint32_t j = 0; j < lvl; ++j) level_destroy(&s->levels[j]);
            return r;
        }
    }

    /* Pool sized for a reasonable initial allocation of cell nodes */
    ae_result_t r = ae_pool_init(&s->node_pool, sizeof(ae_cell_node_t), 4096);
    if (r != AE_OK) {
        for (uint32_t j = 0; j < num_levels; ++j) level_destroy(&s->levels[j]);
        return r;
    }

    return AE_OK;
}

void ae_spatial_destroy(ae_spatial_t *s) {
    if (!s) return;
    for (uint32_t lvl = 0; lvl < s->num_levels; ++lvl)
        level_destroy(&s->levels[lvl]);
    ae_pool_destroy(&s->node_pool);
    memset(s, 0, sizeof(*s));
}

/* =========================================================================
 * Choose level
 *
 * An entity whose AABB spans more than one cell at a given level is placed at
 * the next coarser level.  The finest level is always level (num_levels-1).
 * ========================================================================= */
static int choose_level(const ae_spatial_t *s, const ae_aabb2_t *aabb) {
    double w = aabb->max_x - aabb->min_x;
    double h = aabb->max_y - aabb->min_y;

    /* Scan from finest to coarsest; pick the first level where the AABB fits
     * inside a single cell. */
    for (int lvl = (int)s->num_levels - 1; lvl >= 0; --lvl) {
        const ae_grid_level_t *L = &s->levels[lvl];
        if (w <= L->cell_w && h <= L->cell_h) return lvl;
    }
    return 0; /* coarsest */
}

/* =========================================================================
 * Insert / Remove
 * ========================================================================= */

ae_result_t ae_spatial_insert(ae_spatial_t *s, ae_entity_t entity,
                               const ae_aabb2_t *aabb) {
    if (!s || !aabb) return AE_ERR_INVALID;

    int lvl = choose_level(s, aabb);
    ae_grid_level_t *L = &s->levels[lvl];

    int32_t c0, r0, c1, r1;
    world_to_cell(s, lvl, aabb->min_x, aabb->min_y, &c0, &r0);
    world_to_cell(s, lvl, aabb->max_x, aabb->max_y, &c1, &r1);

    /* Insert into every overlapping cell at the chosen level */
    for (int32_t r = r0; r <= r1; ++r) {
        for (int32_t c = c0; c <= c1; ++c) {
            ae_cell_node_t *node = (ae_cell_node_t *)ae_pool_alloc(&s->node_pool);
            if (!node) return AE_ERR_NOMEM;
            node->entity = entity;
            node->aabb   = *aabb;

            ae_cell_node_t **head = cell_at(L, c, r);
            node->next = *head;
            *head      = node;
        }
    }

    ++s->entity_count;
    return AE_OK;
}

ae_result_t ae_spatial_remove(ae_spatial_t *s, ae_entity_t entity,
                               const ae_aabb2_t *aabb) {
    if (!s || !aabb) return AE_ERR_INVALID;

    int lvl = choose_level(s, aabb);
    ae_grid_level_t *L = &s->levels[lvl];

    int32_t c0, r0, c1, r1;
    world_to_cell(s, lvl, aabb->min_x, aabb->min_y, &c0, &r0);
    world_to_cell(s, lvl, aabb->max_x, aabb->max_y, &c1, &r1);

    bool found = false;

    for (int32_t r = r0; r <= r1; ++r) {
        for (int32_t c = c0; c <= c1; ++c) {
            ae_cell_node_t **pp = cell_at(L, c, r);
            while (*pp) {
                if ((*pp)->entity == entity) {
                    ae_cell_node_t *dead = *pp;
                    *pp = dead->next;
                    ae_pool_free(&s->node_pool, dead);
                    found = true;
                    break;
                }
                pp = &(*pp)->next;
            }
        }
    }

    if (found && s->entity_count > 0) --s->entity_count;
    return found ? AE_OK : AE_ERR_NOT_FOUND;
}

/* =========================================================================
 * Query helpers
 * ========================================================================= */

static ae_result_t result_push(ae_spatial_result_t *r, ae_entity_t e) {
    if (r->count >= r->capacity) {
        uint32_t new_cap = r->capacity ? r->capacity * 2 : 64;
        ae_entity_t *p   = (ae_entity_t *)realloc(r->entities,
                             new_cap * sizeof(ae_entity_t));
        if (!p) return AE_ERR_NOMEM;
        r->entities = p;
        r->capacity = new_cap;
    }
    r->entities[r->count++] = e;
    return AE_OK;
}

static bool aabb_intersect(const ae_aabb2_t *a, const ae_aabb2_t *b) {
    return a->min_x <= b->max_x && a->max_x >= b->min_x &&
           a->min_y <= b->max_y && a->max_y >= b->min_y;
}

/* =========================================================================
 * Query
 * ========================================================================= */

ae_result_t ae_spatial_query(const ae_spatial_t *s,
                              const ae_aabb2_t *query_box,
                              ae_spatial_result_t *result) {
    if (!s || !query_box || !result) return AE_ERR_INVALID;

    /* Query all levels: an entity may be at any level depending on its size */
    for (uint32_t lvl = 0; lvl < s->num_levels; ++lvl) {
        const ae_grid_level_t *L = &s->levels[lvl];

        int32_t c0, r0, c1, r1;
        double qminx = query_box->min_x;
        double qminy = query_box->min_y;
        double qmaxx = query_box->max_x;
        double qmaxy = query_box->max_y;

        /* clamp to world */
        if (qmaxx < s->world_min_x || qminx > s->world_max_x) continue;
        if (qmaxy < s->world_min_y || qminy > s->world_max_y) continue;

        /* Convert query bounds to cell indices */
        double rel_x0 = qminx - s->world_min_x;
        double rel_y0 = qminy - s->world_min_y;
        double rel_x1 = qmaxx - s->world_min_x;
        double rel_y1 = qmaxy - s->world_min_y;

        c0 = clamp_i((int32_t)(rel_x0 / L->cell_w), 0, (int32_t)L->cols - 1);
        r0 = clamp_i((int32_t)(rel_y0 / L->cell_h), 0, (int32_t)L->rows - 1);
        c1 = clamp_i((int32_t)(rel_x1 / L->cell_w), 0, (int32_t)L->cols - 1);
        r1 = clamp_i((int32_t)(rel_y1 / L->cell_h), 0, (int32_t)L->rows - 1);

        for (int32_t r = r0; r <= r1; ++r) {
            for (int32_t c = c0; c <= c1; ++c) {
                const ae_cell_node_t *node = *cell_at(L, c, r);
                while (node) {
                    if (aabb_intersect(&node->aabb, query_box)) {
                        ae_result_t rv = result_push(result, node->entity);
                        if (rv != AE_OK) return rv;
                    }
                    node = node->next;
                }
            }
        }
    }

    return AE_OK;
}

ae_result_t ae_spatial_query_circle(const ae_spatial_t *s,
                                     double cx, double cy, double radius,
                                     ae_spatial_result_t *result) {
    ae_aabb2_t box = {
        .min_x = cx - radius, .min_y = cy - radius,
        .max_x = cx + radius, .max_y = cy + radius
    };
    return ae_spatial_query(s, &box, result);
}

/* =========================================================================
 * Result management
 * ========================================================================= */

void ae_spatial_result_init(ae_spatial_result_t *r) {
    r->entities = NULL;
    r->count    = 0;
    r->capacity = 0;
}

void ae_spatial_result_free(ae_spatial_result_t *r) {
    free(r->entities);
    r->entities = NULL;
    r->count = r->capacity = 0;
}

void ae_spatial_result_clear(ae_spatial_result_t *r) {
    r->count = 0;
}
