/**
 * ae_ecs.c — Archetype-less ECS implementation
 *
 * Key invariants:
 *  - sparse[i] == AE_SPARSE_INVALID  ⟺  entity i does not have this component
 *  - dense[j]  == entity-index that owns slot j
 *  - data + j*comp_size             == component data for dense[j]
 *
 * On removal (entity k with dense index d):
 *   1. Copy dense[count-1] and data[count-1] over index d.
 *   2. Update sparse for the moved entity to point to d.
 *   3. Set sparse[k] = INVALID.
 *   4. Decrement count.
 */
#include "ae_ecs.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* =========================================================================
 * Internal component meta
 * ========================================================================= */
#define COMP_NAME_MAX 64

typedef struct {
    size_t comp_size;
    char   name[COMP_NAME_MAX];
    bool   registered;
} ae_comp_meta_t;

/* =========================================================================
 * World
 * ========================================================================= */
struct ae_world {
    /* Entity storage */
    uint32_t     *gen;           /* generation per entity slot              */
    uint32_t     *free_stack;    /* recycled entity indices                 */
    uint32_t      free_top;      /* top of free stack                       */
    uint32_t      entity_cap;    /* total slots allocated                   */
    uint32_t      entity_count;  /* live entity count                       */
    uint32_t      next_index;    /* next unused slot (before recycling)     */

    /* Component storage */
    ae_comp_meta_t  comp_meta[AE_MAX_COMPONENTS];
    ae_sparse_set_t comp_sets[AE_MAX_COMPONENTS];
    ae_comp_mask_t *entity_mask; /* per-entity component bitmask            */
    uint16_t        comp_count;
    uint16_t        max_comp;

    /* Memory */
    ae_arena_t arena;
};

/* =========================================================================
 * Sparse set helpers
 * ========================================================================= */

static ae_result_t ss_ensure_sparse(ae_sparse_set_t *ss, uint32_t idx) {
    if (idx < ss->sparse_cap) return AE_OK;

    uint32_t new_cap = ss->sparse_cap ? ss->sparse_cap * 2 : 256;
    while (new_cap <= idx) new_cap *= 2;

    uint32_t *s = (uint32_t *)realloc(ss->sparse,
                                       new_cap * sizeof(uint32_t));
    if (!s) return AE_ERR_NOMEM;

    /* Mark new slots as invalid */
    memset(s + ss->sparse_cap, 0xFF,
           (new_cap - ss->sparse_cap) * sizeof(uint32_t));
    ss->sparse     = s;
    ss->sparse_cap = new_cap;
    return AE_OK;
}

static ae_result_t ss_ensure_dense(ae_sparse_set_t *ss) {
    if (ss->count < ss->dense_cap) return AE_OK;

    uint32_t new_cap = ss->dense_cap ? ss->dense_cap * 2 : 256;

    uint32_t *d = (uint32_t *)realloc(ss->dense,
                                       new_cap * sizeof(uint32_t));
    if (!d) return AE_ERR_NOMEM;
    ss->dense = d;

    uint32_t *g = (uint32_t *)realloc(ss->gen,
                                       new_cap * sizeof(uint32_t));
    if (!g) return AE_ERR_NOMEM;
    ss->gen = g;

    uint8_t *dat = (uint8_t *)realloc(ss->data,
                                       (size_t)new_cap * ss->comp_size);
    if (!dat) return AE_ERR_NOMEM;
    ss->data      = dat;
    ss->dense_cap = new_cap;
    return AE_OK;
}

static void ss_destroy(ae_sparse_set_t *ss) {
    free(ss->sparse);
    free(ss->dense);
    free(ss->gen);
    free(ss->data);
    memset(ss, 0, sizeof(*ss));
}

/* =========================================================================
 * World creation / destruction
 * ========================================================================= */

ae_world_t *ae_world_create(uint32_t max_entities, uint16_t max_comp_types) {
    if (max_entities == 0) max_entities = 65536;
    if (max_comp_types == 0 || max_comp_types > AE_MAX_COMPONENTS)
        max_comp_types = AE_MAX_COMPONENTS;

    ae_world_t *w = (ae_world_t *)calloc(1, sizeof(ae_world_t));
    if (!w) return NULL;

    w->entity_cap  = max_entities;
    w->max_comp    = max_comp_types;
    w->comp_count  = 0;
    w->next_index  = 0;
    w->free_top    = 0;

    w->gen = (uint32_t *)calloc(max_entities, sizeof(uint32_t));
    if (!w->gen) goto fail;

    w->free_stack = (uint32_t *)malloc(max_entities * sizeof(uint32_t));
    if (!w->free_stack) goto fail;

    w->entity_mask = (ae_comp_mask_t *)calloc(max_entities,
                                               sizeof(ae_comp_mask_t));
    if (!w->entity_mask) goto fail;

    if (ae_arena_init(&w->arena, 1 << 20) != AE_OK) goto fail;

    return w;

fail:
    free(w->gen);
    free(w->free_stack);
    free(w->entity_mask);
    free(w);
    return NULL;
}

void ae_world_destroy(ae_world_t *w) {
    if (!w) return;

    for (int i = 0; i < w->comp_count; ++i)
        ss_destroy(&w->comp_sets[i]);

    ae_arena_destroy(&w->arena);
    free(w->entity_mask);
    free(w->free_stack);
    free(w->gen);
    free(w);
}

/* =========================================================================
 * Entity lifecycle
 * ========================================================================= */

ae_entity_t ae_entity_create(ae_world_t *w) {
    uint32_t idx;

    if (w->free_top > 0) {
        idx = w->free_stack[--w->free_top];
    } else {
        if (w->next_index >= w->entity_cap) return AE_ENTITY_INVALID;
        idx = w->next_index++;
    }

    ++w->entity_count;
    return ae_entity_make(idx, w->gen[idx]);
}

void ae_entity_destroy(ae_world_t *w, ae_entity_t e) {
    if (!ae_entity_alive(w, e)) return;

    uint32_t idx = ae_entity_index(e);

    /* Remove all components */
    for (int cid = 0; cid < w->comp_count; ++cid) {
        if (ae_mask_has(&w->entity_mask[idx], (ae_comp_id_t)cid))
            ae_comp_remove(w, e, (ae_comp_id_t)cid);
    }

    /* Invalidate handle by bumping generation */
    ++w->gen[idx];
    /* Clear the component mask */
    memset(&w->entity_mask[idx], 0, sizeof(ae_comp_mask_t));

    /* Recycle slot */
    w->free_stack[w->free_top++] = idx;
    --w->entity_count;
}

bool ae_entity_alive(const ae_world_t *w, ae_entity_t e) {
    if (e == AE_ENTITY_INVALID) return false;
    uint32_t idx = ae_entity_index(e);
    uint32_t gen = ae_entity_gen(e);
    if (idx >= w->entity_cap) return false;
    return w->gen[idx] == gen;
}

/* =========================================================================
 * Component registration
 * ========================================================================= */

ae_comp_id_t ae_comp_register(ae_world_t *w, size_t comp_size,
                               const char *name) {
    if (!w || comp_size == 0) return AE_COMP_INVALID;
    if (w->comp_count >= w->max_comp) return AE_COMP_INVALID;

    ae_comp_id_t cid = (ae_comp_id_t)w->comp_count++;
    ae_comp_meta_t *m = &w->comp_meta[cid];
    m->comp_size  = comp_size;
    m->registered = true;
    if (name)
        strncpy(m->name, name, COMP_NAME_MAX - 1);
    else
        m->name[0] = '\0';

    ae_sparse_set_t *ss = &w->comp_sets[cid];
    memset(ss, 0, sizeof(*ss));
    ss->comp_size = comp_size;

    return cid;
}

const char *ae_comp_name(const ae_world_t *w, ae_comp_id_t cid) {
    if (!w || cid >= w->comp_count) return NULL;
    return w->comp_meta[cid].name;
}

/* =========================================================================
 * Component operations
 * ========================================================================= */

ae_result_t ae_comp_add(ae_world_t *w, ae_entity_t e, ae_comp_id_t cid,
                         const void *data) {
    if (!ae_entity_alive(w, e)) return AE_ERR_INVALID;
    if (cid >= w->comp_count)  return AE_ERR_INVALID;

    uint32_t idx = ae_entity_index(e);
    uint32_t gen = ae_entity_gen(e);

    ae_sparse_set_t *ss = &w->comp_sets[cid];

    /* Grow sparse array if needed */
    ae_result_t r = ss_ensure_sparse(ss, idx);
    if (r != AE_OK) return r;

    /* If already present, just overwrite data */
    if (ss->sparse[idx] != AE_SPARSE_INVALID) {
        uint32_t di = ss->sparse[idx];
        if (data)
            memcpy(ss->data + (size_t)di * ss->comp_size, data, ss->comp_size);
        return AE_OK;
    }

    /* Grow dense arrays if needed */
    r = ss_ensure_dense(ss);
    if (r != AE_OK) return r;

    uint32_t di = ss->count++;
    ss->dense[di] = idx;
    ss->gen[di]   = gen;
    ss->sparse[idx] = di;
    if (data)
        memcpy(ss->data + (size_t)di * ss->comp_size, data, ss->comp_size);
    else
        memset(ss->data + (size_t)di * ss->comp_size, 0, ss->comp_size);

    ae_mask_set(&w->entity_mask[idx], cid);
    return AE_OK;
}

ae_result_t ae_comp_remove(ae_world_t *w, ae_entity_t e, ae_comp_id_t cid) {
    if (!ae_entity_alive(w, e)) return AE_ERR_INVALID;
    if (cid >= w->comp_count)  return AE_ERR_INVALID;

    uint32_t idx = ae_entity_index(e);
    ae_sparse_set_t *ss = &w->comp_sets[cid];

    if (idx >= ss->sparse_cap || ss->sparse[idx] == AE_SPARSE_INVALID)
        return AE_ERR_NOT_FOUND;

    uint32_t di   = ss->sparse[idx];
    uint32_t last = ss->count - 1;

    if (di != last) {
        /* Swap with last element */
        uint32_t last_entity = ss->dense[last];
        ss->dense[di]  = last_entity;
        ss->gen[di]    = ss->gen[last];
        memcpy(ss->data + (size_t)di   * ss->comp_size,
               ss->data + (size_t)last * ss->comp_size,
               ss->comp_size);
        ss->sparse[last_entity] = di;
    }

    ss->sparse[idx] = AE_SPARSE_INVALID;
    --ss->count;

    ae_mask_clear(&w->entity_mask[idx], cid);
    return AE_OK;
}

void *ae_comp_get(const ae_world_t *w, ae_entity_t e, ae_comp_id_t cid) {
    if (!ae_entity_alive(w, e)) return NULL;
    if (cid >= w->comp_count)  return NULL;

    uint32_t idx = ae_entity_index(e);
    const ae_sparse_set_t *ss = &w->comp_sets[cid];

    if (idx >= ss->sparse_cap || ss->sparse[idx] == AE_SPARSE_INVALID)
        return NULL;

    uint32_t di = ss->sparse[idx];
    return ss->data + (size_t)di * ss->comp_size;
}

bool ae_comp_has(const ae_world_t *w, ae_entity_t e, ae_comp_id_t cid) {
    return ae_comp_get(w, e, cid) != NULL;
}

/* =========================================================================
 * Query
 * ========================================================================= */

void ae_query_begin(ae_query_t *q, ae_world_t *w,
                    const ae_comp_mask_t *required) {
    q->world    = w;
    q->required = *required;
    q->idx      = 0;

    /* Find the pivot: the registered component type with the fewest entries */
    ae_comp_id_t pivot  = AE_COMP_INVALID;
    uint32_t     fewest = UINT32_MAX;

    for (uint16_t cid = 0; cid < w->comp_count; ++cid) {
        if (!ae_mask_has(required, (ae_comp_id_t)cid)) continue;
        uint32_t cnt = w->comp_sets[cid].count;
        if (cnt < fewest) {
            fewest = cnt;
            pivot  = (ae_comp_id_t)cid;
        }
    }
    q->pivot = pivot;
}

ae_entity_t ae_query_next(ae_query_t *q) {
    if (q->pivot == AE_COMP_INVALID) return AE_ENTITY_INVALID;

    ae_world_t      *w  = q->world;
    ae_sparse_set_t *ss = &w->comp_sets[q->pivot];

    while (q->idx < ss->count) {
        uint32_t di  = q->idx++;
        uint32_t idx = ss->dense[di];
        uint32_t gen = ss->gen[di];
        ae_entity_t e = ae_entity_make(idx, gen);

        /* Check entity is alive and has all required components */
        if (!ae_entity_alive(w, e)) continue;
        if (ae_mask_contains(&w->entity_mask[idx], &q->required))
            return e;
    }

    return AE_ENTITY_INVALID;
}

/* =========================================================================
 * Misc
 * ========================================================================= */

uint32_t ae_world_entity_count(const ae_world_t *w) {
    return w ? w->entity_count : 0;
}
