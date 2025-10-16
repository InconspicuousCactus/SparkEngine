#pragma once

#include "Spark/ecs/entity.h"
#include "Spark/memory/linear_allocator.h"
#include "Spark/physics/physics.h"

void physics_backend_initialize(linear_allocator_t* allocator);
void physics_backend_shutdown();

void entity_add_physics_components(ecs_world_t* world, entity_t entity);

physics_body_t physics_create_collision_mesh(vec3 position, vec3* vertices, u32* indices, u32 vertex_count, u32 index_count, physics_motion_t motion, physics_layer_t layer);
physics_body_t physics_create_cube_collider(vec3 position, vec3 extents, physics_motion_t motion, physics_layer_t layer);

void entity_add_cube_collider (ecs_world_t* world, entity_t entity, vec3 pos, vec3 extents, physics_motion_t motion, physics_layer_t layer);
void entity_add_collision_mesh(ecs_world_t* world, entity_t entity, vec3 pos, vec3* vertices, u32* indices, u32 vertex_count, u32 index_count, physics_motion_t motion, physics_layer_t layer);

void physics_body_destroy(void* component);
