#pragma once

#include "Spark/containers/unordered_map.h"
#include "Spark/ecs/ecs.h"
#include "Spark/ecs/entity.h"
#include "Spark/memory/linear_allocator.h"

typedef struct ecs_world {
    entity_t entity_count;
    darray_entity_record_t records;
    darray_ecs_component_t components;
    darray_entity_archetype_t archetypes;
    darray_ecs_query_t queries;
    darray_ecs_system_t systems[ECS_PHASE_ENUM_MAX];
    component_singleton_map_t singletons;
} ecs_world_t;

void ecs_world_initialize(linear_allocator_t* allocator);
struct ecs_world* ecs_world_get();
void ecs_world_shutdown();
void ecs_world_progress();

ecs_component_id ecs_world_component_define(ecs_world_t* world, const char* name, u32 stride);

void ecs_world_set_singleton(ecs_world_t* world, ecs_component_id component, entity_t singleton);
void* ecs_world_get_singleton(ecs_world_t* world, ecs_component_id component);

#define ECS_SET_SINGLETON(world, component, entity) ecs_world_set_singleton(world, ECS_COMPONENT_ID(component), entity)
#define ECS_GET_SINGLETON(world, component) (component*)ecs_world_get_singleton(world, ECS_COMPONENT_ID(component))

#ifdef SPARK_DEBUG
#define ECS_COMPONENT_DEFINE(world, component) ECS_COMPONENT_ID(component) = ecs_world_component_define(world, #component, sizeof(component))
#else
#define ECS_COMPONENT_DEFINE(world, component) ECS_COMPONENT_ID(component) = ecs_world_component_define(world, "", sizeof(component))
#endif
