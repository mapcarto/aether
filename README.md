# aetoer
高性能空间金字塔事件驱动引擎

A high-performance spatiotemporal event-driven engine written in pure C.

## Features

| Feature | Detail |
|---|---|
| **Archetype-less ECS** | Sparse-set ECS — O(1) component add / remove / lookup, cache-friendly packed iteration |
| **Pyramid Spatial Grid** | Multi-level regular grid index for 2-D space; place entities at the finest level that fits their AABB; query across all levels |
| **Lock-free Event Bus** | Dmitry Vyukov MPMC ring buffer; CAS-only, no mutexes; concurrent multi-producer / single-consumer |
| **Zero GC** | Pure C with explicit pool and arena allocators; no hidden allocation on hot paths |
| **Memory Transparency** | `ae_arena_t` (bump-pointer) and `ae_pool_t` (slab) — deterministic layout and lifetime |

## Architecture

```
aetoer/
├── include/
│   ├── aetoer.h        — umbrella public header
│   ├── ae_types.h      — common types (ae_entity_t, ae_vec2_t, ae_aabb2_t …)
│   ├── ae_memory.h     — arena + pool allocator API
│   ├── ae_ecs.h        — ECS world, entities, components, queries
│   ├── ae_spatial.h    — pyramid spatial grid index
│   └── ae_event.h      — lock-free MPMC event bus
├── src/
│   ├── ae_memory.c
│   ├── ae_ecs.c
│   ├── ae_spatial.c
│   └── ae_event.c
├── tests/
│   ├── test_memory.c   — arena / pool unit tests
│   ├── test_ecs.c      — ECS unit tests
│   ├── test_spatial.c  — spatial grid unit tests
│   ├── test_event.c    — event bus unit tests (incl. concurrent)
│   └── test_perf.c     — throughput benchmarks
└── CMakeLists.txt
```

## Quick Start

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
ctest              # run all unit tests
./test_perf        # run performance benchmarks
```

### Debug build (AddressSanitizer + UBSanitizer)

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc) && ctest
```

## API Overview

### ECS

```c
ae_world_t *w = ae_world_create(1000000, 256);

// register component types once
ae_comp_id_t cpos = ae_comp_register(w, sizeof(Position), "Position");
ae_comp_id_t cvel = ae_comp_register(w, sizeof(Velocity), "Velocity");

// create entities and attach components
ae_entity_t e = ae_entity_create(w);
Position pos = {1.0f, 2.0f, 0.0f};
ae_comp_add(w, e, cpos, &pos);

// iterate: all entities with both Position and Velocity
ae_comp_mask_t req = {{0}};
ae_mask_set(&req, cpos);
ae_mask_set(&req, cvel);

ae_query_t q;
ae_query_begin(&q, w, &req);
ae_entity_t ent;
while ((ent = ae_query_next(&q)) != AE_ENTITY_INVALID) {
    Position *p = ae_comp_get(w, ent, cpos);
    Velocity *v = ae_comp_get(w, ent, cvel);
    p->x += v->vx;
}

ae_world_destroy(w);
```

### Spatial Index

```c
ae_spatial_t s;
ae_spatial_init(&s, 0, 0, 10000, 10000, 5, 8, 8);

ae_aabb2_t box = {100, 100, 110, 110};
ae_spatial_insert(&s, entity, &box);

ae_spatial_result_t res;
ae_spatial_result_init(&res);
ae_aabb2_t query = {90, 90, 200, 200};
ae_spatial_query(&s, &query, &res);   // res.entities[0..res.count)

ae_spatial_result_free(&res);
ae_spatial_destroy(&s);
```

### Event Bus

```c
ae_event_bus_t bus;
ae_bus_init(&bus, 1u << 20);  // 1 M ring-buffer slots

// subscribe
ae_bus_subscribe(&bus, MY_EVENT_TYPE, my_handler, userdata);

// publish (lock-free, thread-safe)
ae_bus_publish_now(&bus, MY_EVENT_TYPE, source_entity, target_entity,
                   &payload, sizeof(payload));

// drain + dispatch (call from consumer thread)
ae_bus_drain(&bus, 65536);

ae_bus_destroy(&bus);
```

## Design Notes

### Entity ID format
64-bit handle = 32-bit generation (high) | 32-bit index (low).  
Recycled slots get a new generation, so stale handles never silently alias live entities.

### Sparse-set ECS
Each component type owns an independent sparse set: `sparse[entity_index]` → dense array index.  
- Add/remove/lookup: **O(1)**  
- Iteration: **cache-linear** (packed dense + data arrays)  
- No archetypes — any combination of components is valid without schema changes

### Pyramid Spatial Grid
A stack of regular grids (level 0 = coarsest, level N-1 = finest).  
Each level doubles the resolution.  An entity is stored at the finest level
whose cell is large enough to contain its AABB.  Queries scan all levels for
their overlapping cells and filter by AABB intersection.

### Lock-free Ring Buffer (MPMC)
Based on Dmitry Vyukov's bounded MPMC queue (sequence-counter algorithm).  
Each slot carries an atomic `sequence` counter:
- **Producer**: CAS `tail`, write event, store `seq = pos + 1`
- **Consumer**: CAS `head`, dispatch, store `seq = pos + capacity`

No mutex is taken anywhere on the publish/drain path.

## License

MIT

