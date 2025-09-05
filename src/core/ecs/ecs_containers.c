#include "Spark/ecs/ecs.h"
#include "Spark/utils/hashing.h"

set_impl(ecs_component_id, ecs_component_set);
darray_impl(ecs_column_t, ecs_column);
darray_impl(entity_record_t, entity_record);
darray_impl(entity_archetype_t, entity_archetype);
darray_impl(entity_archetype_t*, entity_archetype_ptr);
darray_impl(ecs_system_t, ecs_system);
darray_impl(ecs_component_t, ecs_component);
void darray_entity_archetype_ptr_map_pair_create(
    u32 initial_size, struct darray_entity_archetype_ptr_map_pair *out_array) {
  out_array->capacity = initial_size;
  out_array->count = 0;
  if (!(sizeof(entity_archetype_ptr_map_pair_t) != 0)) {
    pvt_log(LOG_LEVEL_ERROR, "Assertion '"
                             "sizeof(entity_archetype_ptr_map_pair_t) != 0"
                             "' failed: "
                             "Cannot create darray of type '"
                             "entity_archetype_ptr_map_pair_t"
                             "' with size of 0.");
    pvt_log(LOG_LEVEL_ERROR,
            "\t\t"
            "/home/default/Projects/Spark/Engine/src/core/ecs/ecs_containers.c"
            ":%i",
            11);
    abort();
  };
  out_array->data = create_tracked_allocation(
      sizeof(entity_archetype_ptr_map_pair_t) * initial_size, MEMORY_TAG_DARRAY,
      "/home/default/Projects/Spark/Engine/src/core/ecs/ecs_containers.c", 11);
}
void darray_entity_archetype_ptr_map_pair_destroy(
    struct darray_entity_archetype_ptr_map_pair *array) {
  if (!(array->data)) {
    pvt_log(LOG_LEVEL_ERROR, "Assertion '"
                             "array->data"
                             "' failed: "
                             "Cannot operate on null darray");
    pvt_log(LOG_LEVEL_ERROR,
            "\t\t"
            "/home/default/Projects/Spark/Engine/src/core/ecs/ecs_containers.c"
            ":%i",
            11);
    abort();
  };
  if (array->data) {
    free_tracked_allocation((void *)array->data,
                            sizeof(entity_archetype_ptr_map_pair_t) *
                                array->capacity,
                            MEMORY_TAG_DARRAY);
    array->data = ((void *)0);
  }
}
void darray_entity_archetype_ptr_map_pair_reserve(
    struct darray_entity_archetype_ptr_map_pair *array, u32 size);
entity_archetype_ptr_map_pair_t *darray_entity_archetype_ptr_map_pair_push(
    struct darray_entity_archetype_ptr_map_pair *array,
    const entity_archetype_ptr_map_pair_t value) {
  if (!(array)) {
    pvt_log(LOG_LEVEL_ERROR, "Assertion '"
                             "array"
                             "' failed: "
                             "Cannot operate on null darray");
    pvt_log(LOG_LEVEL_ERROR,
            "\t\t"
            "/home/default/Projects/Spark/Engine/src/core/ecs/ecs_containers.c"
            ":%i",
            11);
    abort();
  };
  if (!(array->data)) {
    pvt_log(LOG_LEVEL_ERROR, "Assertion '"
                             "array->data"
                             "' failed: "
                             "Cannot operate on uninitialized");
    pvt_log(LOG_LEVEL_ERROR,
            "\t\t"
            "/home/default/Projects/Spark/Engine/src/core/ecs/ecs_containers.c"
            ":%i",
            11);
    abort();
  };
  if (array->count >= array->capacity) {
    darray_entity_archetype_ptr_map_pair_reserve(array, array->capacity * 2);
  }
  u32 array_index = array->count;
  array->count++;
  scopy_memory(&array->data[array_index], &value,
               sizeof(entity_archetype_ptr_map_pair_t));
  return &array->data[array_index];
}
void darray_entity_archetype_ptr_map_pair_push_range(
    struct darray_entity_archetype_ptr_map_pair *array, u32 count,
    const entity_archetype_ptr_map_pair_t *values) {
  if (!(array->data)) {
    pvt_log(LOG_LEVEL_ERROR, "Assertion '"
                             "array->data"
                             "' failed: "
                             "Cannot operate on null darray");
    pvt_log(LOG_LEVEL_ERROR,
            "\t\t"
            "/home/default/Projects/Spark/Engine/src/core/ecs/ecs_containers.c"
            ":%i",
            11);
    abort();
  };
  scopy_memory(array->data + array->count, values,
               count * sizeof(entity_archetype_ptr_map_pair_t));
  array->count += count;
}
entity_archetype_ptr_map_pair_t darray_entity_archetype_ptr_map_pair_pop(
    struct darray_entity_archetype_ptr_map_pair *array, u32 index) {
  if (!(array->data)) {
    pvt_log(LOG_LEVEL_ERROR, "Assertion '"
                             "array->data"
                             "' failed: "
                             "Cannot operate on null darray");
    pvt_log(LOG_LEVEL_ERROR,
            "\t\t"
            "/home/default/Projects/Spark/Engine/src/core/ecs/ecs_containers.c"
            ":%i",
            11);
    abort();
  };
  if (!(index >= 0 && index < array->count)) {
    pvt_log(LOG_LEVEL_ERROR,
            "Assertion '"
            "index >= 0 && index < array->count"
            "' failed: "
            "Darray tring to pop out of bounds index: %d. Count: %d",
            index, array->count);
    pvt_log(LOG_LEVEL_ERROR,
            "\t\t"
            "/home/default/Projects/Spark/Engine/src/core/ecs/ecs_containers.c"
            ":%i",
            11);
    abort();
  };
  entity_archetype_ptr_map_pair_t value = array->data[index];
  if (index < array->count) {
    for (u32 i = index; i < array->count - 1; i++) {
      array->data[i] = array->data[i - 1];
    }
  }
  array->count--;
  return value;
}
void darray_entity_archetype_ptr_map_pair_pop_range(
    struct darray_entity_archetype_ptr_map_pair *array, u32 count,
    u32 start_index) {
  if (!(array->data)) {
    pvt_log(LOG_LEVEL_ERROR, "Assertion '"
                             "array->data"
                             "' failed: "
                             "Cannot operate on null darray");
    pvt_log(LOG_LEVEL_ERROR,
            "\t\t"
            "/home/default/Projects/Spark/Engine/src/core/ecs/ecs_containers.c"
            ":%i",
            11);
    abort();
  };
  if (!(start_index >= 0 && start_index + count < array->count)) {
    pvt_log(LOG_LEVEL_ERROR,
            "Assertion '"
            "start_index >= 0 && start_index + count < array->count"
            "' failed: "
            "Unable to pop range of values in darray. Start Index: %d, Pop "
            "Count: %d, Array Count: %d",
            start_index, count, array->count);
    pvt_log(LOG_LEVEL_ERROR,
            "\t\t"
            "/home/default/Projects/Spark/Engine/src/core/ecs/ecs_containers.c"
            ":%i",
            11);
    abort();
  };
  if (start_index + count < array->count) {
    scopy_memory(&array->data[start_index], &array->data[start_index + count],
                 sizeof(entity_archetype_ptr_map_pair_t) * array->count -
                     (start_index + count));
  }
  array->count -= count;
}
void darray_entity_archetype_ptr_map_pair_reserve(
    struct darray_entity_archetype_ptr_map_pair *array, u32 size) {
  if (!(sizeof(entity_archetype_ptr_map_pair_t) != 0)) {
    pvt_log(LOG_LEVEL_ERROR, "Assertion '"
                             "sizeof(entity_archetype_ptr_map_pair_t) != 0"
                             "' failed: "
                             "Cannot create darray of type '"
                             "entity_archetype_ptr_map_pair_t"
                             "' with size of 0.");
    pvt_log(LOG_LEVEL_ERROR,
            "\t\t"
            "/home/default/Projects/Spark/Engine/src/core/ecs/ecs_containers.c"
            ":%i",
            11);
    abort();
  };
  if (array->capacity >= size) {
    return;
  }
  entity_archetype_ptr_map_pair_t *temp = create_tracked_allocation(
      sizeof(entity_archetype_ptr_map_pair_t) * size, MEMORY_TAG_DARRAY,
      "/home/default/Projects/Spark/Engine/src/core/ecs/ecs_containers.c", 11);
  scopy_memory(temp, array->data,
               array->count * sizeof(entity_archetype_ptr_map_pair_t));
  free_tracked_allocation((void *)array->data,
                          sizeof(entity_archetype_ptr_map_pair_t) *
                              array->capacity,
                          MEMORY_TAG_DARRAY);
  array->data = temp;
  array->capacity = size;
}
void darray_entity_archetype_ptr_map_pair_clear(
    struct darray_entity_archetype_ptr_map_pair *array) {
  if (!(array->data)) {
    pvt_log(LOG_LEVEL_ERROR, "Assertion '"
                             "array->data"
                             "' failed: "
                             "Cannot operate on null darray");
    pvt_log(LOG_LEVEL_ERROR,
            "\t\t"
            "/home/default/Projects/Spark/Engine/src/core/ecs/ecs_containers.c"
            ":%i",
            11);
    abort();
  };
  array->count = 0;
};
void entity_archetype_ptr_map_create(u32 capacity,
                                     entity_archetype_ptr_map_t *out_hashmap) {
  out_hashmap->capacity = capacity;
  out_hashmap->pairs = create_tracked_allocation(
      sizeof(darray_entity_archetype_ptr_map_pair_t) * capacity,
      MEMORY_TAG_ARRAY,
      "/home/default/Projects/Spark/Engine/src/core/ecs/ecs_containers.c", 11);
  for (u32 i = 0; i < capacity; i++) {
    darray_entity_archetype_ptr_map_pair_create(4, &out_hashmap->pairs[i]);
  }
}
void entity_archetype_ptr_map_destroy(
    const entity_archetype_ptr_map_t *hashmap) {
  for (u32 i = 0; i < hashmap->capacity; i++) {
    darray_entity_archetype_ptr_map_pair_destroy(&hashmap->pairs[i]);
  }
  free_tracked_allocation((void *)hashmap->pairs,
                          sizeof(darray_entity_archetype_ptr_map_pair_t) *
                              hashmap->capacity,
                          MEMORY_TAG_ARRAY);
}
void entity_archetype_ptr_map_remove(const entity_archetype_ptr_map_t *hashmap,
                                     ecs_component_id key) {
  hash_t hash = hash_passthrough(key);
  u32 index = hash % hashmap->capacity;
  darray_entity_archetype_ptr_map_pair_t *pairs = &hashmap->pairs[index];
  for (u32 i = 0; i < pairs->count; i++) {
    if (u64_compare(pairs->data[i].key, key)) {
      darray_entity_archetype_ptr_map_pair_pop(pairs, i);
      return;
    }
  }
  pvt_log(LOG_LEVEL_ERROR, "Hashmap does not contain key.");
}
void entity_archetype_ptr_map_insert(const entity_archetype_ptr_map_t *hashmap,
                                     ecs_component_id key,
                                     entity_archetype_t *value) {
  hash_t hash = hash_passthrough(key);
  u32 index = hash % hashmap->capacity;
  entity_archetype_ptr_map_pair_t pair = {
      .value = value,
      .key = key,
  };
  darray_entity_archetype_ptr_map_pair_push(&hashmap->pairs[index], pair);
}
b8 entity_archetype_ptr_map_contains(const entity_archetype_ptr_map_t *hashmap,
                                     ecs_component_id key) {
  hash_t hash = hash_passthrough(key);
  u32 index = hash % hashmap->capacity;
  darray_entity_archetype_ptr_map_pair_t *pairs = &hashmap->pairs[index];
  for (u32 i = 0; i < pairs->count; i++) {
    if (u64_compare(pairs->data[i].key, key)) {
      return 1;
    }
  }
  return 0;
}
b8 entity_archetype_ptr_map_try_get(const entity_archetype_ptr_map_t *hashmap,
                                    ecs_component_id key,
                                    entity_archetype_t **out_value) {
  hash_t hash = hash_passthrough(key);
  u32 index = hash % hashmap->capacity;
  darray_entity_archetype_ptr_map_pair_t *pairs = &hashmap->pairs[index];
  for (u32 i = 0; i < pairs->count; i++) {
    if (u64_compare(pairs->data[i].key, key)) {
      *out_value = pairs->data[i].value;
      return 1;
    }
  }
  return 0;
}
entity_archetype_t **
entity_archetype_ptr_map_get(const entity_archetype_ptr_map_t *hashmap,
                             ecs_component_id key) {
  hash_t hash = hash_passthrough(key);
  u32 index = hash % hashmap->capacity;
  darray_entity_archetype_ptr_map_pair_t *pairs = &hashmap->pairs[index];
  for (u32 i = 0; i < pairs->count; i++) {
    if (u64_compare(pairs->data[i].key, key)) {
      return &pairs->data[i].value;
    }
  }
  return ((void *)0);
};

darray_impl(entity_t, entity);
