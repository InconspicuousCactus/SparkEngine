#include "Spark/ecs/entity.h"
#include "Spark/defines.h"
#include "Spark/ecs/components/entity_child.h"
#include "Spark/ecs/components/entity_parent.h"
#include "Spark/ecs/ecs.h"
#include "Spark/ecs/ecs_world.h"
#include "Spark/math/mat4.h"
#include "Spark/types/transforms.h"
#include <stdlib.h>

// =========================
// Private functions
// =========================
void entity_transition_archetype(struct ecs_world* world, 
        entity_t entity, 
        entity_archetype_t* dest_archetype);

entity_t entity_create(struct ecs_world* world) {
    entity_t entity = world->entity_count++;
    entity_record_t record = {
        .archetype_index = 0,
        .index = world->archetypes.data[0].columns.count,
    };
    darray_entity_push(&world->archetypes.data[0].entities, entity);
    darray_entity_record_push(&world->records, record);

    return entity;
}

b8 entity_has_component(struct ecs_world* world, entity_t entity, ecs_index component) {
    entity_record_t record = world->records.data[entity];
    entity_archetype_t* archetype = &world->archetypes.data[record.archetype_index];

    return ecs_component_set_contains(&archetype->component_set, component);
}

void* entity_get_component(struct ecs_world* world, entity_t entity, ecs_index component) {
    if (entity == INVALID_ID) {
        SWARN("Trying to get component from invalid entity id");
        return NULL;
    }

    entity_record_t record = world->records.data[entity];
    entity_archetype_t* archetype = &world->archetypes.data[record.archetype_index];

    if (!ecs_component_set_contains(&archetype->component_set, component)) {
// #ifdef SPARK_DEBUG
//         SERROR("Failed to get component '%s' from entity 0x%x.", world->components.data[component].name, entity);
// #else
//         SERROR("Failed to get component from entity 0x%x.", entity);
// #endif
        return NULL;
    }

    u32 component_column_index = ecs_component_set_get_index(&archetype->component_set, component);
    ecs_column_t column = archetype->columns.data[component_column_index];

    return column.data + record.index * column.component_stride; 
}

b8 entity_try_get_component(struct ecs_world* world, entity_t entity, ecs_index component, void** out_data) {
    SASSERT(entity >= 0 && entity < world->records.count, "Cannot access invalid entity %d. 0 <= %d < %d (entity / record count)", entity, entity, world->records.data->index);
    entity_record_t record = world->records.data[entity];
    entity_archetype_t* archetype = &world->archetypes.data[record.archetype_index];

    if (!ecs_component_set_contains(&archetype->component_set, component)) {
        return false;
    }

    u32 component_column_index = ecs_component_set_get_index(&archetype->component_set, component);
    ecs_column_t column = archetype->columns.data[component_column_index];

    *out_data = column.data + record.index * column.component_stride;
    return true;
}

void entity_add_component(struct ecs_world* world, entity_t entity, ecs_component_id component_id) {
    if (entity_has_component(world, entity, component_id)) {
        return;
    }

    entity_record_t record = world->records.data[entity];
    entity_archetype_t* current_archetype = &world->archetypes.data[record.archetype_index];

    entity_archetype_t* new_archetype = NULL;
    if (entity_archetype_ptr_map_try_get(&current_archetype->edges.add_edges, component_id, &new_archetype)) {
        // New archetype should be acquired, just need to transition between them
        entity_transition_archetype(world, entity, new_archetype);

        // Reserve space for the new component
        u32 column_index = ecs_component_set_get_index(&new_archetype->component_set, component_id);
        ecs_column_t* column = &new_archetype->columns.data[column_index];
        if (column->count >= column->capacity - 1) {
            ecs_component_column_resize(column, column->capacity * ECS_COLUMN_RESIZE_FACTOR);
        }
        column->count++;

        return;
    }

    // Check if the new archetype exists
    u32 target_component_count = current_archetype->component_set.count + 1;
    ecs_component_t component = world->components.data[component_id];

    for (u32 i = 0; i < component.archetypes.count; i++) {
        entity_archetype_t* archetype = component.archetypes.data[i];

        // Size check
        if (archetype->component_set.count != target_component_count || 
                !ecs_component_set_contains(&archetype->component_set, component_id)) {
            continue;
        }

        // Check if the source archetype is null, if so, just set the archetype
        if (current_archetype->archetype_id == 0) {
            new_archetype = archetype;
            break;
        }

        // Type check
        b8 is_existing_archetype = true;
        for (u32 j = 0; j < current_archetype->component_set.capacity; j++) {
            ecs_component_id component = current_archetype->component_set.data[j].value;
            if (component == INVALID_ID) {
                continue;
            }
            if (!ecs_component_set_contains(&archetype->component_set, component)) {
                is_existing_archetype = false;
                break;
            }
        }

        if (!is_existing_archetype) {
            continue;
        }

        new_archetype = archetype;
        break;
    }

    if (!new_archetype) {
        // Failed to find new archetype, need to create it
        new_archetype = entity_archetype_create_from_base(world, &world->archetypes.data[record.archetype_index], 1, &component_id);
        entity_archetype_match_queryies(new_archetype, world);
    }

    // Add edges
    entity_archetype_ptr_map_insert(&world->archetypes.data[record.archetype_index].edges.add_edges, component_id, new_archetype);
    entity_archetype_ptr_map_insert(&new_archetype->edges.remove_edges, component_id, current_archetype);

    // New archetype should be acquired, just need to transition between them
    entity_transition_archetype(world, entity, new_archetype);

    // Add new component to new archetype column
    u32 new_column_index = ecs_component_set_get_index(&new_archetype->component_set, component_id);
    SASSERT(new_column_index != INVALID_ID, "Failed to get component id of new archetype set.");
    ecs_column_t* column = &new_archetype->columns.data[new_column_index];
    if (column->count >= column->capacity - 1) {
        ecs_component_column_resize(column, column->capacity * ECS_COLUMN_RESIZE_FACTOR);
    }

    column->count++;
}

void entity_transition_archetype(struct ecs_world* world, 
        entity_t entity, 
        entity_archetype_t* dest_archetype) {
    entity_record_t* record = &world->records.data[entity];
    entity_archetype_t* source_archetype = &world->archetypes.data[record->archetype_index];
    ecs_index entity_row = record->index;

    // Move entity from one archetype to the next
    darray_entity_pop(&source_archetype->entities, entity_row);
    ecs_index future_index = dest_archetype->entities.count;
    darray_entity_push(&dest_archetype->entities, entity);

    // Append each component from source to dest
    for (u32 i = 0; i < source_archetype->component_set.capacity; i++) {
        ecs_component_id component = source_archetype->component_set.data[i].value;
        if (component == INVALID_ID) {
            continue;
        }
        u32 source_column_index = ecs_component_set_get_index(&source_archetype->component_set, component);
        u32 dest_column_index = ecs_component_set_get_index(&dest_archetype->component_set, component);

        ecs_column_t* source_column = &source_archetype->columns.data[source_column_index];
        void* source_data = source_column->data + entity_row * source_column->component_stride;
        ecs_component_column_push(&dest_archetype->columns.data[dest_column_index], source_data);

        // Remove data from source archetype
        ecs_component_column_pop(&source_archetype->columns.data[source_column_index], entity_row);
    }

    // Update the record
    record->index = future_index;
    record->archetype_index = dest_archetype->archetype_id;
}

void entity_set_component(struct ecs_world* world, entity_t entity, ecs_component_id component, void* data, u32 stride) {
    if (!entity_has_component(world, entity, component)) {
        entity_add_component(world, entity, component);
    }

    entity_record_t record = world->records.data[entity];
    u32 column_index = ecs_component_set_get_index(&world->archetypes.data[record.archetype_index].component_set, component);
    SASSERT(column_index != INVALID_ID, "Cannot set component %s to entity %d when entity does not have component.", world->components.data[component].name, entity);
    scopy_memory(world->archetypes.data[record.archetype_index].columns.data[column_index].data + record.index * stride, data, stride);
}

void entity_add_transforms(ecs_world_t* world, entity_t entity, vec3 position, vec3 scale, quat rotation) {
    translation_t tr = { .value = position };
    scale_t sc = { .value = scale };
    rotation_t rot = { .value = rotation };

    ENTITY_SET_COMPONENT(world, entity, translation_t, tr);
    ENTITY_SET_COMPONENT(world, entity, scale_t, sc);
    ENTITY_SET_COMPONENT(world, entity, rotation_t, rot);
    ENTITY_SET_COMPONENT(world, entity, local_to_world_t, { mat4_translation(position) });
    ENTITY_SET_COMPONENT(world, entity, dirty_transform_t, { true });
}


void entity_add_child(struct ecs_world* world, entity_t parent, entity_t child) {
    // Check if child has a parent
    // Remove child from that parent if so
    entity_parent_t* old_parent = ENTITY_GET_COMPONENT(world, child, entity_parent_t);
    if (old_parent) {
        entity_child_t* old_parent_children = ENTITY_GET_COMPONENT(world, old_parent->parent, entity_child_t);
        darray_entity_pop(&old_parent_children->children, old_parent->self_index);
    }

    // Get the child's parent component

    // Check if new parent has a children array
    // If so, just append the child
    entity_child_t* children = ENTITY_GET_COMPONENT(world, parent, entity_child_t);
    if (children) {
        entity_parent_t parent_relationship = {
            .parent = parent,
            .self_index = children->children.count,
        };
        darray_entity_push(&children->children, child);
        ENTITY_SET_COMPONENT(world, child, entity_parent_t, parent_relationship);
        entity_parent_t* p;
        ENTITY_TRY_GET_COMPONENT(world, child, entity_parent_t, &p);
        return;
    }

    // Otherwise, create a new child array
    darray_entity_t new_children;
    darray_entity_create(4, &new_children);
    darray_entity_push(&new_children, child);

    // Set the parent's children array
    ENTITY_SET_COMPONENT(world, parent, entity_child_t, { new_children });

    // Set the child's parent component
    entity_parent_t parent_relationship = {
        .parent = parent,
        .self_index = 0,
    };

    ENTITY_SET_COMPONENT(world, child, entity_parent_t, parent_relationship);
}

