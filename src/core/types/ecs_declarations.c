#include "Spark/ecs/ecs.h"
#include "Spark/ecs/ecs_world.h"
#include "Spark/physics/physics.h"
#include "Spark/physics/physics_backend.h"
#include "Spark/renderer/material.h"
#include "Spark/renderer/mesh.h"
#include "Spark/types/aabb.h"
#include "Spark/types/camera.h"
#include "Spark/types/transforms.h"
#include "Spark/ecs/components/entity_child.h"
#include "Spark/ecs/components/entity_parent.h"

// Rendering
ECS_COMPONENT_DECLARE(camera_t);
ECS_COMPONENT_DECLARE(orthographic_camera_t);
ECS_COMPONENT_DECLARE(mesh_t);
ECS_COMPONENT_DECLARE(material_t);

// Culling
ECS_COMPONENT_DECLARE(aabb_t);

// Transforms
ECS_COMPONENT_DECLARE(dirty_transform_t);
ECS_COMPONENT_DECLARE(translation_t);
ECS_COMPONENT_DECLARE(rotation_t);
ECS_COMPONENT_DECLARE(scale_t);
ECS_COMPONENT_DECLARE(local_to_world_t);

ECS_COMPONENT_DECLARE(translation_2d_t);
ECS_COMPONENT_DECLARE(scale_2d_t);
// ECS_COMPONENT_DECLARE(local_to_world_2d_t);

ECS_COMPONENT_DECLARE(entity_parent_t);
ECS_COMPONENT_DECLARE(entity_child_t);

// Physics
ECS_COMPONENT_DECLARE(physics_body_t);
ECS_COMPONENT_DECLARE(velocity_t);

void ecs_register_types(ecs_world_t* world) {
    // Rendering
    ECS_COMPONENT_DEFINE(world, camera_t);
    ECS_COMPONENT_DEFINE(world, orthographic_camera_t);
    ECS_COMPONENT_DEFINE(world, mesh_t);
    ECS_COMPONENT_DEFINE(world, material_t);

    // Culling
    ECS_COMPONENT_DEFINE(world, aabb_t);

    // Transforms
    ECS_COMPONENT_DEFINE(world, dirty_transform_t);
    ECS_COMPONENT_DEFINE(world, translation_t);
    ECS_COMPONENT_DEFINE(world, rotation_t);
    ECS_COMPONENT_DEFINE(world, scale_t);
    ECS_COMPONENT_DEFINE(world, local_to_world_t);

    ECS_COMPONENT_DEFINE(world, translation_2d_t);
    ECS_COMPONENT_DEFINE(world, scale_2d_t);
    // ECS_COMPONENT_DEFINE(world, local_to_world_2d_t);

    ECS_COMPONENT_DEFINE(world, entity_parent_t);
    ECS_COMPONENT_DEFINE(world, entity_child_t);

    ECS_COMPONENT_ADD_DESTRUCTOR(world, entity_child_t, entity_child_destroy_callback);

    // Physics
    ECS_COMPONENT_DEFINE(world, physics_body_t);
    ECS_COMPONENT_DEFINE(world, velocity_t);

    ECS_COMPONENT_ADD_DESTRUCTOR(world, physics_body_t, physics_body_destroy);
}
