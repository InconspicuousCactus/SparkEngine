#include "Spark/ecs/ecs.h"
#include "Spark/utils/hashing.h"

set_impl(ecs_component_id, ecs_component_set);
darray_impl(ecs_column_t, ecs_column);
darray_impl(entity_record_t, entity_record);
darray_impl(entity_archetype_t, entity_archetype);
darray_impl(entity_archetype_t*, entity_archetype_ptr);
darray_impl(ecs_system_t, ecs_system);
darray_impl(ecs_component_t, ecs_component);
hashmap_impl(entity_archetype_ptr_map, ecs_component_id, entity_archetype_t*, hash_passthrough, u64_compare);

darray_impl(entity_t, entity);
