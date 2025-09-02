#pragma once
#include "Spark/containers/darray.h"
#include "Spark/containers/generic/darray_ints.h"
#include "Spark/containers/set.h"
#include "Spark/containers/unordered_map.h"
#include "Spark/ecs/entity.h"


// ================================
// Forward declarations
// ================================
struct ecs_world;
struct entity_archetype;

set_header(ecs_component_id, ecs_component_set);

// ================================
// ECS Phases
// ================================
typedef enum ecs_phase {
    ECS_PHASE_PHYSICS, 
    ECS_PHASE_PRE_UPDATE, 
    ECS_PHASE_UPDATE, 
    ECS_PHASE_POST_UPDATE, 
    ECS_PHASE_TRANSFORM, 
    ECS_PHASE_PRE_RENDER, 
    ECS_PHASE_RENDER, 
    ECS_PHASE_POST_RENDER, 
    ECS_PHASE_ENUM_MAX,
} ecs_phase_t;

// ================================
// Column
// ================================
typedef struct ecs_column {
    void* data;
    ecs_index component_stride;
    ecs_index count;
    ecs_index capacity;
} ecs_column_t;

#define ECS_COLUMN_RESIZE_FACTOR 2
void ecs_component_column_create(u32 initial_count, u32 component_stride, ecs_column_t* out_column);
void ecs_component_column_destroy(ecs_column_t* column);
void ecs_component_column_resize(ecs_column_t* column, u32 size);
void ecs_component_column_push(ecs_column_t* column, void* data);
void ecs_component_column_pop (ecs_column_t* column, ecs_index row);

darray_header(ecs_column_t, ecs_column);

// ================================
// ECS Record
// ================================
typedef struct ecs_record {
    ecs_index index;
    u32 archetype_index;
} entity_record_t;
darray_header(entity_record_t, entity_record);

// ================================
// Entity Archetype Edge
// ================================
struct entity_archetype;
darray_header(struct entity_archetype*, entity_archetype_ptr);
hashmap_header(entity_archetype_ptr_map, ecs_component_id, struct entity_archetype*);

typedef struct entity_archetype_edge {
    entity_archetype_ptr_map_t add_edges;
    entity_archetype_ptr_map_t remove_edges;
} entity_archetype_edge_t;

// ================================
// Entity Archetype
// ================================
typedef struct entity_archetype {
    ecs_component_set_t component_set;
    darray_ecs_column_t columns;
    darray_entity_t entities;
    entity_archetype_edge_t edges;
    ecs_index archetype_id;
} entity_archetype_t  ; 

void entity_archetype_create(struct ecs_world* world, u32 component_count, ecs_component_id* components, entity_archetype_t* out_archetype);
entity_archetype_t* entity_archetype_create_from_base(struct ecs_world* world, entity_archetype_t* base_archetype, u32 component_count, ecs_component_id* components);
void entity_archetype_destroy(entity_archetype_t* archetype);
void entity_archetype_print_debug(entity_archetype_t* archetype);
void entity_archetype_match_queryies(entity_archetype_t* archetyle, struct ecs_world* world);

darray_header(entity_archetype_t, entity_archetype);

// ================================
// ECS Component
// ================================
typedef struct ecs_component {
    darray_entity_archetype_ptr_t archetypes;
    u32 stride;
#ifdef SPARK_DEBUG
    const char* name;
#endif
} ecs_component_t;
darray_header(ecs_component_t, ecs_component);

// ================================
// ECS iterator 
// ================================
typedef struct ecs_iterator {
    ecs_world_t* world;
    void** component_data;
    entity_archetype_t* archetype;
    u32 component_count;
    u32 entity_count;
} ecs_iterator_t;

#define ECS_ITERATOR_GET_COMPONENTS(iterator, index) (iterator->component_data[index]); SASSERT(index < iterator->component_count, "Cannot get component at index %d from query with %d components", index, iterator->component_count)
// void* ecs_iterator_get_type(ecs_iterator_t* iterator, ecs_component_id component);

// ================================
// ECS query 
// ================================
typedef struct ecs_query_create_info {
    u32 component_count;
    u32 without_component_count;
    const ecs_component_id* components;
    const ecs_component_id* without_components;
} ecs_query_create_info_t;

typedef struct ecs_query {
    darray_u32_t archetype_indices;
    darray_u32_t components;
    darray_u32_t without_components;
    u32 hash;
    ecs_world_t* world;
} ecs_query_t;

darray_header(ecs_query_t, ecs_query);
#define MAX_QUERY_COMPONENT_COUNT 32

ecs_query_t* ecs_query_create(struct ecs_world* world, const ecs_query_create_info_t* create_info);
void ecs_query_destroy(ecs_query_t* query);
b8 ecs_query_matches_archetype(ecs_query_t* query, entity_archetype_t* archetype);
void ecs_query_create_iterator(ecs_query_t* query, ecs_iterator_t* out_iterator);
void ecs_query_iterate(ecs_query_t* query, void (iterate_function)(ecs_iterator_t* iterator));

// ================================
// ECS system
// ================================
typedef struct ecs_system {
    ecs_query_t* query;
    void (*callback)(ecs_iterator_t* iterator);
#ifdef SPARK_DEBUG
    const char* name;
    f64 runtime;
    u32 calls;
#endif
} ecs_system_t;

void ecs_system_create(struct ecs_world* world, ecs_phase_t phase, const ecs_query_create_info_t* create_info, void (*callback)(ecs_iterator_t*), const char* name);
void ecs_system_destroy(ecs_system_t* system);

darray_header(ecs_system_t, ecs_system);

// ================================
// Utility Macros
// ================================
#define ECS_COMPONENT_ID(component) ECS_##component##_ID
#define ECS_COMPONENT_DECLARE(component) ecs_component_id ECS_COMPONENT_ID(component)
#define ECS_SYSTEM_CREATE(world, phase, components, callback) ecs_system_create(world, phase, sizeof(components) / sizeof(ecs_component_id), callback)

