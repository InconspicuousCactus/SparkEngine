#include "Spark/memory/linear_allocator.h"

#include "Spark/core/smemory.h"
#include "Spark/core/logging.h"

void linear_allocator_create(u64 total_size, void* memory, linear_allocator_t* out_allocator) {
    if (out_allocator) {
        out_allocator->total_size = total_size;
        out_allocator->allocated = 0;
        // out_allocator->owns_memory = memory == NULL;
        // if (memory) {
        //     out_allocator->memory = memory;
        // } else {
        out_allocator->memory = sallocate(total_size, MEMORY_TAG_ALLOCATOR);
        // }
    }
}
void linear_allocator_destroy(linear_allocator_t* allocator) {
    SDEBUG("Shutting down linear allocator 1. %p", allocator);
    if (allocator) {
        allocator->allocated = 0;
        // if (allocator->owns_memory && allocator->memory) {
        sfree(allocator->memory, allocator->total_size, MEMORY_TAG_ALLOCATOR);
        // } 
        allocator->memory = 0;
        allocator->total_size = 0;
        allocator->owns_memory = false;
    }
}

void* linear_allocator_allocate(linear_allocator_t* allocator, u64 size) {
    if (allocator && allocator->memory) {
        if (allocator->allocated + size > allocator->total_size) {
            SERROR("linear_allocator_allocate - Tried to allocate %luB, only %luB remaining.", size, allocator->total_size - allocator->allocated);
            return 0;
        }

        void* block = ((u8*)allocator->memory) + allocator->allocated;
        allocator->allocated += size;
        return block;
    }

    SERROR("linear_allocator_allocate - provided allocator not initialized.");
    return 0;
}

void linear_allocator_free_all(linear_allocator_t* allocator) {
    if (allocator && allocator->memory) {
        allocator->allocated = 0;
        szero_memory(allocator->memory, allocator->total_size);
    }
}
