#pragma once

#include "Spark/ecs/entity.h"
#include "Spark/memory/linear_allocator.h"
#include "Spark/physics/physics.h"

void physics_backend_initialize(linear_allocator_t* allocator);
void physics_backend_shutdown();

void entity_add_cube_collider(ecs_world_t* world, entity_t entity, vec3 extents, physics_motion_t motion, physics_layer_t layer);

void physics_body_destroy(void* component);
