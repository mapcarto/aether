/**
 * aetoer.h — Public umbrella header for the aetoer engine
 *
 * Include this single header to access all engine subsystems:
 *   - ECS  : ae_world_t, ae_entity_t, component management
 *   - Spatial: ae_spatial_t pyramid grid index
 *   - Event  : ae_event_bus_t lock-free event bus
 *   - Memory : ae_arena_t, ae_pool_t
 *
 * Example quick-start:
 *
 *   #include "aetoer.h"
 *
 *   ae_world_t *w = ae_world_create(1000000, 256);
 *   ae_entity_t e = ae_entity_create(w);
 *   // ...
 *   ae_world_destroy(w);
 */
#ifndef AETOER_H
#define AETOER_H

#include "ae_types.h"
#include "ae_memory.h"
#include "ae_ecs.h"
#include "ae_spatial.h"
#include "ae_event.h"

/* ---- version ------------------------------------------------------------- */
#define AETOER_VERSION_MAJOR 0
#define AETOER_VERSION_MINOR 1
#define AETOER_VERSION_PATCH 0

#define AETOER_VERSION_STRING "0.1.0"

#endif /* AETOER_H */
