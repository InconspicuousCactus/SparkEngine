#pragma once

#include "Spark/core/logging.h"
#include "Spark/core/smemory.h"
#include "Spark/containers/darray.h"
#include "Spark/utils/hashing.h"

#define HASHMAP_MAX_LINKED_LIST_LENGTH 3

#define hashmap_type(name, key_type, value_type)                                                     \
    typedef struct name##_pair {                                                                     \
        key_type key;                                                                                \
        value_type value;                                                                            \
    } name##_pair_t;                                                                                 \
                                                                                                     \
    darray_header(name##_pair_t, name##_pair);                                                       \
                                                                                                     \
    typedef struct name {                                                                            \
        u32 capacity;                                                                                \
        darray_##name##_pair_t* pairs;                                                               \
    } name##_t;

#define hashmap_header(name, key_type, value_type)                                                   \
    hashmap_type(name, key_type, value_type);                                                        \
    void name##_create(u32 capacity, name##_t* out_hashmap);                                         \
    void name##_destroy(const name##_t* hashmap);                                                           \
    void name##_insert(const name##_t* hashmap, key_type key, value_type value);                           \
    b8 name##_contains(const name##_t* hashmap, key_type key);                                             \
    b8 name##_try_get(const name##_t* hashmap, key_type key, value_type* out_value);                       \
    value_type* name##_get(const name##_t* hashmap, key_type key);                                         

// Key copy is a function in the form of void (key_type* dest, key_type* src);
// If key_copy == NULL, the key wil take a simple copy (i.e.) dest = src;
#define hashmap_impl(name, key_type, value_type, hash_function, key_compare_function, key_copy_function)                                      \
    darray_impl(name##_pair_t, name##_pair);                                                         \
    void name##_create(u32 capacity, name##_t* out_hashmap) {                                        \
        out_hashmap->capacity = capacity;                                                            \
        out_hashmap->pairs = sallocate(sizeof(darray_##name##_pair_t) * capacity, MEMORY_TAG_ARRAY); \
        for (u32 i = 0; i < capacity; i++) {                                                         \
            darray_##name##_pair_create(4, &out_hashmap->pairs[i]);                                  \
        }                                                                                            \
    }                                                                                                \
    void name##_destroy(const name##_t* hashmap) {                                                         \
        for (u32 i = 0; i < hashmap->capacity; i++) {                                                \
            darray_##name##_pair_destroy(&hashmap->pairs[i]);                                        \
        }                                                                                            \
        sfree(hashmap->pairs, sizeof(darray_##name##_pair_t) * hashmap->capacity, MEMORY_TAG_ARRAY); \
    }                                                                                                \
    void name##_remove(const name##_t* hashmap, key_type key) {                                            \
        hash_t hash = hash_function(key);                                                            \
        u32 index = hash % hashmap->capacity;                                                        \
                                                                                                     \
        darray_##name##_pair_t* pairs = &hashmap->pairs[index];                                      \
        for (u32 i = 0; i < pairs->count; i++) {                                                     \
            if (key_compare_function(pairs->data[i].key, key)) {                                                         \
                darray_##name##_pair_pop(pairs, i);                                                  \
                return;                                                                              \
            }                                                                                        \
        }                                                                                            \
                                                                                                     \
        SERROR("Hashmap does not contain key.");                                                     \
    }                                                                                                \
                                                                                                     \
    void name##_insert(const name##_t* hashmap, key_type key, value_type value) {                          \
        hash_t hash = hash_function(key);                                                            \
        u32 index = hash % hashmap->capacity;                                                        \
                                                                                                     \
        name##_pair_t pair = {                                                                       \
            .value = value,                                                                          \
            .key = key_copy_function(key), \
        };                                                                                           \
        darray_##name##_pair_push(&hashmap->pairs[index], pair);                                     \
    }                                                                                                \
    \
    b8 name##_contains(const name##_t* hashmap, key_type key) {                                            \
        hash_t hash = hash_function(key);                                                            \
        u32 index = hash % hashmap->capacity;                                                        \
        \
        darray_##name##_pair_t* pairs = &hashmap->pairs[index];                                      \
        for (u32 i = 0; i < pairs->count; i++) {                                                     \
            if (key_compare_function(pairs->data[i].key, key)) {                                                         \
                return true;                                                                         \
            }                                                                                        \
        }                                                                                            \
        return false;                                                                                \
    }                                                                                                \
    \
    b8 name##_try_get(const name##_t* hashmap, key_type key, value_type* out_value) {                      \
        hash_t hash = hash_function(key);                                                            \
        u32 index = hash % hashmap->capacity;                                                        \
        \
        darray_##name##_pair_t* pairs = &hashmap->pairs[index];                                      \
        for (u32 i = 0; i < pairs->count; i++) {                                                     \
            if (key_compare_function(pairs->data[i].key, key)) {                                                         \
                *out_value = pairs->data[i].value;                                                   \
                return true;                                                                         \
            }                                                                                        \
        }                                                                                            \
        return false;                                                                                \
    }                                                                                                \
    \
    value_type* name##_get(const name##_t* hashmap, key_type key) {                                        \
        hash_t hash = hash_function(key);                                                            \
        u32 index = hash % hashmap->capacity;                                                        \
        \
        darray_##name##_pair_t* pairs = &hashmap->pairs[index];                                      \
        for (u32 i = 0; i < pairs->count; i++) {                                                     \
            if (key_compare_function(pairs->data[i].key, key)) {                                                         \
                return &pairs->data[i].value;                                                        \
            }                                                                                        \
        }                                                                                            \
        return NULL;                                                                                 \
    }
