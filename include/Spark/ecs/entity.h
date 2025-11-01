#pragma once

// ================================
// ECS Base Types
// ================================
#include "Spark/containers/darray.h"
#include "Spark/math/math_types.h"

typedef u64 ecs_index;
typedef ecs_index entity_t;
typedef u32 ecs_component_id;

darray_header(entity_t, entity);

#define ENTITY_SET_COMPONENT(world, entity, component, ...) \
{ \
    component __val__ = (component)__VA_ARGS__; \
    entity_set_component(world, entity, ECS_COMPONENT_ID(component), &__val__, sizeof(component)); \
} 
#define ENTITY_ADD_COMPONENT(world, entity, component) \
    entity_add_component(world, entity, ECS_COMPONENT_ID(component))
#define ENTITY_GET_COMPONENT(world, entity, component) \
    (component*)entity_get_component(world, entity, ECS_COMPONENT_ID(component))
#define ENTITY_TRY_GET_COMPONENT(world, entity, component, out_value) entity_try_get_component(world, entity, ECS_COMPONENT_ID(component), (void**)out_value)
#define ENTITY_HAS_COMPONENT(world, entity, component) \
    entity_has_component(world, entity, ECS_COMPONENT_ID(component))

struct ecs_world;
typedef struct ecs_world ecs_world_t;

entity_t entity_create(struct ecs_world* world);
b8 entity_has_component(struct ecs_world* world, entity_t entity, ecs_index component);
b8 entity_try_get_component(struct ecs_world* world, entity_t entity, ecs_index component, void** out_value);
void* entity_get_component(struct ecs_world* world, entity_t entity, ecs_index component);
void entity_add_component(struct ecs_world* world, entity_t entity, ecs_component_id component_id);
void entity_set_component(struct ecs_world* world, entity_t entity, ecs_component_id component, void* data, u32 stride);

void entity_add_transforms(ecs_world_t* world, entity_t entity, vec3 position, vec3 scale, quat rotation);
void entity_add_child(struct ecs_world* world, entity_t parent, entity_t child);
