#include "Spark/core/smemory.h"
#include "Spark/memory/freelist.h"
#include "Spark/platform/platform.h"
#include "Spark/math/smath.h"
#include <string.h>

typedef struct freelist_explicit {
    struct freelist_explicit* next;
    struct freelist_explicit* previous;
} freelist_explicit_t;

void freelist_create(void* memory, u64 memory_size, freelist_t* out_allocator) {
    SASSERT(memory_size >= FREELIST_MINIMUM_MEMORY_SIZE, "Cannot create freelist with memory size less than %d", FREELIST_MINIMUM_MEMORY_SIZE);
    out_allocator->memory = memory;

    u32 usable_size = memory_size - sizeof(freelist_block_t) * 4;

    // Create first block
    freelist_block_t* block = out_allocator->memory + sizeof(freelist_block_t);
    block->size             = usable_size;
    block->allocated        = false;

    freelist_block_t* block_end = out_allocator->memory + sizeof(freelist_block_t) + usable_size;
    block_end->size             = usable_size;
    block_end->allocated        = false;

    freelist_explicit_t* explicit = (void*)block + sizeof(freelist_block_t);
    explicit->next     = NULL;
    explicit->previous = NULL;
    out_allocator->first_free_block  = explicit;

    // Set end of memory block
    freelist_block_t* leading_block = out_allocator->memory;
    leading_block->size = 0;
    leading_block->allocated = true;

    freelist_block_t* trailing_block = out_allocator->memory + memory_size - sizeof(freelist_block_t);
    trailing_block->size = 0;
    trailing_block->allocated = true;
}

void freelist_destroy(freelist_t* allocator) {
    platform_free(allocator->memory, true);
}

void* freelist_allocate(freelist_t* allocator, u64 size) {
    SASSERT(allocator->memory, "Freelist has not been initialized or was not assigned memory.");
    size = smax(size, 32);

    freelist_explicit_t* explicit = allocator->first_free_block;
    while (explicit) {
        freelist_block_t* block = (void*)allocator->first_free_block - sizeof(freelist_block_t);

        // Check if block can contain allocation
        if (size > block->size) {
            explicit = explicit->next;
            continue;
        }

        // It can
        // Split the block into the allocation and a new block
        const u32 remaining_size = block->size - size;

        const u8 create_new_block = sizeof(freelist_block_t) < remaining_size;
        if (create_new_block) {
            freelist_block_t* new_block = ((void*)block) + sizeof(freelist_block_t) * 2 + size;
            freelist_explicit_t* new_explicit = (void*)new_block + sizeof(freelist_block_t);
            new_block->size = remaining_size - sizeof(freelist_block_t) * 2;
            new_explicit->next = explicit->next;
            new_explicit->previous = explicit->previous;

            if (explicit->next) {
                explicit->next->previous = new_explicit;
            }
            if (explicit->previous) {
                explicit->previous->next = new_explicit;
            }

            // NOTE: Following block is equivalent and is marginally faster
            // if (explicit == allocator->first_free_block) {
            //     allocator->first_free_block = new_explicit;
            // }
            const u8 replace_first_block = explicit == allocator->first_free_block;
            allocator->first_free_block = (void*)((size_t)allocator->first_free_block * (!replace_first_block)
                + (size_t)new_explicit * replace_first_block);
        } else {
            allocator->first_free_block = explicit->next;
        }



        // Create a new allocation
        freelist_block_t* block_end = ((void*)block) + sizeof(freelist_block_t) + size;
        block_end->size      = size;
        block_end->allocated = true;

        block->size          = size;
        block->allocated     = true;

        return ((void*)block) + sizeof(freelist_block_t);
    }

    return NULL;
}

void freelist_free(freelist_t* allocator, void* address) {
    // get allocation
    freelist_block_t* block = address - sizeof(freelist_block_t);
    freelist_block_t* block_header = address + block->size;

    // Convert allocation to block
    enum coalesce_state {
        coalesce_state_default = 0,
        coalesce_state_previous = 1,
        coalesce_state_next = 2,
        coalesce_state_all = 3,
    };

    freelist_explicit_t* new_explicit = address;
    freelist_block_t* previous_block = address - sizeof(freelist_block_t) * 2;
    freelist_explicit_t* previous_explicit = (void*)previous_block - previous_block->size;
    freelist_block_t* next_block = address + block->size + sizeof(freelist_block_t);
    freelist_explicit_t* next_explicit = (void*)next_block + sizeof(freelist_block_t);

    enum coalesce_state state = 0;
    if (!previous_block->allocated) {
        state += coalesce_state_previous;
    }
    if (!next_block->allocated) {
        state += coalesce_state_next;
    }

    switch (state) {
        case coalesce_state_default:
            block->allocated = false;
            block_header->allocated = false;
            break;
        case coalesce_state_previous:
            previous_block = (void*)previous_block - previous_block->size - sizeof(freelist_block_t);
            previous_block->size += block->size + sizeof(freelist_block_t) * 2;
            block = previous_block;
            block_header->size = block->size;
            block_header->allocated = false;

            if (previous_explicit->previous) {
                previous_explicit->previous->next = previous_explicit->next;
            } else {
                allocator->first_free_block = NULL;
            }
            if (previous_explicit->next) {
                previous_explicit->next->previous = previous_explicit->previous;
            }
            new_explicit = previous_explicit;
            break;
        case coalesce_state_next:
            block->allocated = false;
            block->size += next_block->size + sizeof(freelist_block_t) * 2;
            block_header = (void*)block_header + next_block->size + sizeof(freelist_block_t) * 2;
            block_header->size = block->size;

            if (next_explicit->previous) {
                next_explicit->previous->next = next_explicit->next;
            } else {
                allocator->first_free_block = NULL;
            }

            if (next_explicit->next) {
                next_explicit->next->previous = next_explicit->previous;
            }
            break;
        case coalesce_state_all:
            previous_block = (void*)previous_block - previous_block->size - sizeof(freelist_block_t);
            previous_block->size += block->size + next_block->size + sizeof(freelist_block_t) * 4;
            block = previous_block;
            block_header = (void*)previous_block + previous_block->size + sizeof(freelist_block_t);
            block_header->size = block->size;

            if (next_explicit->previous) {
                next_explicit->previous->next = next_explicit->next;
            }
            if (next_explicit->next) {
                next_explicit->next->previous = next_explicit->previous;
            }

            if (previous_explicit->previous) {
                previous_explicit->previous->next = previous_explicit->next;
            }
            if (previous_explicit->next) {
                previous_explicit->next->previous = previous_explicit->previous;
            }

            if (allocator->first_free_block == previous_explicit) {
                allocator->first_free_block = NULL;
            }
            if (allocator->first_free_block == next_explicit) {
                allocator->first_free_block = NULL;
            }

            new_explicit = previous_explicit;
            break;
    }

    new_explicit->next                    = allocator->first_free_block;
    new_explicit->previous                = NULL;
    if (allocator->first_free_block) {
        allocator->first_free_block->previous = new_explicit;
    }
    allocator->first_free_block           = new_explicit;
}
