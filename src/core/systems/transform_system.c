#include "Spark/defines.h"
#include "Spark/ecs/components/entity_child.h"
#include "Spark/ecs/components/entity_parent.h"
#include "Spark/ecs/entity.h"
#include "Spark/systems/core_systems.h"
#include "Spark/ecs/ecs.h"
#include "Spark/ecs/ecs_world.h"
#include "Spark/math/mat4.h"
#include "Spark/math/quat.h"
#include "Spark/types/transforms.h"

// void create_world_to_local_matrix(ecs_iterator_t* iterator) {
//
//     translation_t* translations = ECS_ITERATOR_GET_COMPONENTS(iterator, 0);
//     rotation_t* rotations = ECS_ITERATOR_GET_COMPONENTS(iterator, 1);
//     scale_t* scales = ECS_ITERATOR_GET_COMPONENTS(iterator, 2);
//     local_to_world_t* locals = ECS_ITERATOR_GET_COMPONENTS(iterator, 3);
//
//     for (u32 i = 0; i < iterator->entity_count; i++) {
//         mat4 translation = mat4_translation(translations[i].value);
//         mat4 rotation = quat_to_mat4(rotations[i].value);
//         mat4 scale = mat4_scale(scales[i].value);
//
//         mat4 local = mat4_identity();
//         local = mat4_mul(local, rotation);
//         local = mat4_mul(local, scale);
//         local = mat4_mul(local, translation);
//
//
//         locals[i].value = local;
//     }
// }

void create_2d_world_to_local_matrix(ecs_iterator_t* iterator) {

    translation_2d_t* translations = ECS_ITERATOR_GET_COMPONENTS(iterator, 0);
    rotation_t* rotations = ECS_ITERATOR_GET_COMPONENTS(iterator, 1);
    scale_2d_t* scales = ECS_ITERATOR_GET_COMPONENTS(iterator, 2);
    local_to_world_t* locals = ECS_ITERATOR_GET_COMPONENTS(iterator, 3);

    for (u32 i = 0; i < iterator->entity_count; i++) {
        mat4 translation = mat4_translation((vec3) {.x = translations[i].value.x, .y = translations[i].value.y });
        mat4 rotation = quat_to_mat4(rotations[i].value);
        mat4 scale = mat4_scale((vec3) { .x = scales[i].value.x, .y = scales[i].value.y });

        mat4 local = mat4_identity();
        local = mat4_mul(local, rotation);
        local = mat4_mul(local, scale);
        local = mat4_mul(local, translation);

        
        locals[i].value = local;
    }
}

void parent_entity(ecs_world_t* world, mat4 parent_matrix, entity_t child, b8 force_dirty) {
    translation_t* tr;
    rotation_t* rot;
    scale_t* scale;
    dirty_transform_t* dirty;

    if (!ENTITY_TRY_GET_COMPONENT(world, child, translation_t, &tr) ||
            !ENTITY_TRY_GET_COMPONENT(world, child, dirty_transform_t, &dirty) ||
            !ENTITY_TRY_GET_COMPONENT(world, child, rotation_t, &rot) ||
            !ENTITY_TRY_GET_COMPONENT(world, child, scale_t, &scale)) {
        return;
    }

    mat4 world_matrix = mat4_identity();
    if (dirty->dirty || force_dirty) {
        force_dirty = true;
        mat4 translation = mat4_translation(tr->value);
        mat4 rotation = quat_to_mat4(rot->value);
        mat4 scale_matrix = mat4_scale(scale->value);

        world_matrix = mat4_mul(world_matrix, rotation);
        world_matrix = mat4_mul(world_matrix, scale_matrix);
        world_matrix = mat4_mul(world_matrix, translation);

        world_matrix = mat4_mul(world_matrix, parent_matrix);
        ENTITY_SET_COMPONENT(world, child, local_to_world_t, { world_matrix });
        dirty->dirty = false;
    }

    // Check for children
    entity_child_t* children;
    if (!ENTITY_TRY_GET_COMPONENT(world, child, entity_child_t, &children)) {
        return;
    }

    for (u32 i = 0; i < children->children.count; i++) {
        parent_entity(world, world_matrix, children->children.data[i], force_dirty);
    }
}

void create_world_to_local_matrix(ecs_iterator_t* iterator) {
    translation_t* translations = ECS_ITERATOR_GET_COMPONENTS(iterator, 0);
    rotation_t* rotations = ECS_ITERATOR_GET_COMPONENTS(iterator, 1);
    scale_t* scales = ECS_ITERATOR_GET_COMPONENTS(iterator, 2);
    local_to_world_t* locals = ECS_ITERATOR_GET_COMPONENTS(iterator, 3);
    dirty_transform_t* dirty = ECS_ITERATOR_GET_COMPONENTS(iterator, 4);

    for (u32 i = 0; i < iterator->entity_count; i++) {
        // if (!dirty[i].dirty) {
        //     continue;
        // }

        SASSERT(!ecs_component_set_contains(&iterator->archetype->component_set, ECS_COMPONENT_ID(entity_parent_t)), "local to world system cannot run on child objects.");
        mat4 translation = mat4_translation(translations[i].value);
        mat4 rotation    = quat_to_mat4(rotations[i].value);
        mat4 scale       = mat4_scale(scales[i].value);

        mat4 local = mat4_identity();
        local      = mat4_mul(local, rotation);
        local      = mat4_mul(local, scale);
        local      = mat4_mul(local, translation);


        locals[i].value = local;
        dirty[i].dirty = false;
    }

    u32 child_index = ecs_component_set_get_index(&iterator->archetype->component_set, ECS_COMPONENT_ID(entity_child_t));
    if (child_index == INVALID_ID) {
        return;
    }

    // Check for children
    entity_child_t* children = iterator->archetype->columns.data[child_index].data;
    for (u32 i = 0; i < iterator->entity_count; i++) {
        entity_child_t* child_array = &children[i];
        for (u32 j = 0; j < child_array->children.count; j++) {
            parent_entity(iterator->world, locals[i].value, child_array->children.data[j], false);
        }
    }
}

void transform_system_initialize(ecs_world_t* world) {
    const ecs_system_create_info_t update_2d_create_info = {
        .query = {
            .component_count = 4,
            .components = (ecs_component_id[]) {
                ECS_COMPONENT_ID(translation_2d_t),
                ECS_COMPONENT_ID(rotation_t),
                ECS_COMPONENT_ID(scale_2d_t),
                ECS_COMPONENT_ID(local_to_world_t)
            }
        },
        .phase = ECS_PHASE_TRANSFORM,
        .name = "Transform Update 2D",
        .callback = create_world_to_local_matrix,
    };
    ecs_system_create(world, &update_2d_create_info);

    const ecs_system_create_info_t update_3d_create_info = {
        .query = {
            .component_count = 5,
            .components = (ecs_component_id[]) {
                ECS_COMPONENT_ID(translation_t),
                ECS_COMPONENT_ID(rotation_t),
                ECS_COMPONENT_ID(scale_t),
                ECS_COMPONENT_ID(local_to_world_t),
                ECS_COMPONENT_ID(dirty_transform_t),
            }
        },
        .phase = ECS_PHASE_TRANSFORM,
        .name = "Transform Update 3D",
        .callback = create_world_to_local_matrix,
    };
    ecs_system_create(world, &update_3d_create_info);

}

