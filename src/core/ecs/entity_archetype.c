#include "Spark/containers/generic/darray_ints.h"
#include "Spark/defines.h"
#include "Spark/ecs/ecs.h"
#include "Spark/ecs/ecs_world.h"

#include "Spark/ecs/entity.h"
#include "Spark/math/smath.h"

#define EDGE_MAP_DEFAULT_CAPACITY 30

void entity_archetype_create(struct ecs_world* world, u32 component_count, ecs_component_id* components, entity_archetype_t* out_archetype) {
    out_archetype->archetype_id = world->archetypes.count;

    darray_entity_create(32, &out_archetype->entities);
    if (component_count > 0) {
        darray_ecs_column_create(component_count, &out_archetype->columns);
    }
    ecs_component_set_create(smax(component_count, 1), &out_archetype->component_set);

    entity_archetype_ptr_map_create(EDGE_MAP_DEFAULT_CAPACITY , &out_archetype->edges.add_edges);
    entity_archetype_ptr_map_create(EDGE_MAP_DEFAULT_CAPACITY, &out_archetype->edges.remove_edges);

    // Manually set component_set data
    for (u32 i = 0; i < component_count; i++) {
        ecs_component_set_insert(&out_archetype->component_set, components[i]);
    }
    out_archetype->component_set.count = component_count;

    // Initialize all columns
    for (u32 i = 0; i < component_count; i++) {
        ecs_component_t* component = &world->components.data[components[i]];
        ecs_component_column_create(1, component->stride, &out_archetype->columns.data[i]);
    }
    out_archetype->columns.count = component_count;

    // Check if archetype matches any existing queries
    for (u32 i = 0; i < world->queries.count; i++) {
        if (ecs_query_matches_archetype(&world->queries.data[i], out_archetype)) {
            darray_u32_push(&world->queries.data[i].archetype_indices, out_archetype->archetype_id);
        }
    }
}

entity_archetype_t* entity_archetype_create_from_base(struct ecs_world* world, entity_archetype_t* base_archetype, u32 component_count, ecs_component_id* components) {
    u32 archetype_id = world->archetypes.count;
    entity_archetype_t* out_archetype = darray_entity_archetype_push(&world->archetypes, (entity_archetype_t) {});
    out_archetype->archetype_id = archetype_id;

    darray_entity_create(32, &out_archetype->entities);

    u32 total_component_count = component_count + base_archetype->component_set.count;
    if (total_component_count > 0) {
        darray_ecs_column_create(total_component_count, &out_archetype->columns);
    }
    ecs_component_set_create(smax(total_component_count, 1), &out_archetype->component_set);
    entity_archetype_ptr_map_create(EDGE_MAP_DEFAULT_CAPACITY, &out_archetype->edges.add_edges);
    entity_archetype_ptr_map_create(EDGE_MAP_DEFAULT_CAPACITY, &out_archetype->edges.remove_edges);

    // Manually set component_set data
    for (u32 i = 0; i < base_archetype->component_set.capacity; i++) {
        ecs_component_id component = base_archetype->component_set.data[i].value;
        if (component == INVALID_ID) {
            continue;
        }
        ecs_component_set_insert(&out_archetype->component_set, component);
    }

    for (u32 i = 0; i < component_count; i++) {
        ecs_component_set_insert(&out_archetype->component_set, components[i]);
    }

    // Initialize columns from base
    for (u32 i = 0; i < out_archetype->component_set.capacity; i++) {
         u32 value = out_archetype->component_set.data[i].value;
         u32 index = out_archetype->component_set.data[i].index;

         if (value == INVALID_ID) {
             continue;
         }

         ecs_component_t* component = &world->components.data[value];
         ecs_component_column_create(1, component->stride, &out_archetype->columns.data[index]);

         darray_entity_archetype_ptr_push(&component->archetypes, out_archetype);
    }

    out_archetype->columns.count = total_component_count;
    return out_archetype;
}

void entity_archetype_destroy(entity_archetype_t* archetype) {
    ecs_component_set_destroy(&archetype->component_set);
    if (archetype->columns.data) {
        for (u32 i = 0; i < archetype->columns.count; i++) {
            ecs_component_column_destroy(&archetype->columns.data[i]);
        }
        darray_ecs_column_destroy(&archetype->columns);
    }

    entity_archetype_ptr_map_destroy(&archetype->edges.add_edges);
    entity_archetype_ptr_map_destroy(&archetype->edges.remove_edges);
    darray_entity_destroy(&archetype->entities);
}

void entity_archetype_print_debug(entity_archetype_t* archetype) {
#ifdef SPARK_DEBUG
    ecs_world_t* world = ecs_world_get();

    SDEBUG("ARCHETYPE: %d", archetype->archetype_id);
    for (u32 i = 0; i < archetype->columns.count; i++) {
        u32 component_index = archetype->component_set.data[i].value;
        SDEBUG("Component %d: %s (Index: %d)", i, world->components.data[component_index].name, component_index);
    }
#endif
}


void entity_archetype_match_queryies(entity_archetype_t *archetype, struct ecs_world *world) {
    // Check if archetype matches any existing queries
    for (u32 i = 0; i < world->queries.count; i++) {
        if (ecs_query_matches_archetype(&world->queries.data[i], archetype)) {
            darray_u32_push(&world->queries.data[i].archetype_indices, archetype->archetype_id);
        }
    }
}
