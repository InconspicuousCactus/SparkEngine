#pragma once

#include "Spark/core/smemory.h"
#include "Spark/threading/mutex.h"

#define GENERAL_ALLOCATOR_DEFAULT_SIZE     (256 * MB)
#define FREELIST_MINIMUM_MEMORY_SIZE 256
#define FREELIST_ALLOCATED_MAGIC 0x2651FE91
#define FREELIST_FREE_MAGIC  0x91EF27E9

typedef struct freelist_block {
    u32 size;
    u32 allocated;
} freelist_block_t;

struct freelist_explicit;

typedef struct freelist {
    void* memory;
    struct freelist_explicit* first_free_block;
    spark_mutex_t mutex;
#ifdef SPARK_DEBUG
    u64 capacity;
#endif
// #if SPARK_DEBUG
//     struct freelist_block* first_block;
// #endif
} freelist_t;

void freelist_create(void* memory, u64 memory_size, freelist_t* out_allocator);
void freelist_destroy(freelist_t* allocator);

void* freelist_allocate(freelist_t* allocator, u64 size);
void freelist_free(freelist_t* allocator, void* address);

#ifdef SPARK_DEBUG
void freelist_check_health(freelist_t* allocator);
#endif
