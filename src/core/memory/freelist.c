#include "Spark/core/smemory.h"
#include "Spark/memory/freelist.h"
#include "Spark/defines.h"
#include "Spark/platform/platform.h"
#include "Spark/math/smath.h"
#include "Spark/threading/mutex.h"
#include <stdio.h>
#include <string.h>

// This is here just for debugging purposes when things go to hell. 
// The 40+ asserts add a lot of overhead so this macro is used just to switch them on / off
#define freelist_asserts 0
#if freelist_asserts == 1
#define _SASSERT(...) SASSERT(__VA_ARGS__)
#else
#define _SASSERT(...)
#endif

typedef struct freelist_explicit {
    struct freelist_explicit* next;
    struct freelist_explicit* previous;
} freelist_explicit_t;

void freelist_create(void* memory, u64 memory_size, freelist_t* out_allocator) {
    _SASSERT(memory_size >= FREELIST_MINIMUM_MEMORY_SIZE, "Cannot create freelist with memory size less than %d", FREELIST_MINIMUM_MEMORY_SIZE);
    out_allocator->memory = memory;

    u32 usable_size = memory_size - sizeof(freelist_block_t) * 4;

    // Create first block
    freelist_block_t* block = out_allocator->memory + sizeof(freelist_block_t);
    block->size             = usable_size;
    block->allocated        = FREELIST_FREE_MAGIC;

    freelist_block_t* block_end = out_allocator->memory + sizeof(freelist_block_t) * 2 + usable_size;
    block_end->size             = usable_size;
    block_end->allocated        = FREELIST_FREE_MAGIC;

    freelist_explicit_t* explicit = (void*)block + sizeof(freelist_block_t);
    explicit->next     = NULL;
    explicit->previous = NULL;
    out_allocator->first_free_block  = explicit;

    // Set end of memory block
    freelist_block_t* leading_block = out_allocator->memory;
    leading_block->size = 0;
    leading_block->allocated = FREELIST_ALLOCATED_MAGIC;

    freelist_block_t* trailing_block = out_allocator->memory + memory_size - sizeof(freelist_block_t);
    trailing_block->size = 0;
    trailing_block->allocated = FREELIST_ALLOCATED_MAGIC;
#ifdef SPARK_DEBUG
    out_allocator->capacity = memory_size;
#endif

    mutex_create(&out_allocator->mutex);
}

void freelist_destroy(freelist_t* allocator) {
    mutex_destroy(&allocator->mutex);
}

void* freelist_allocate(freelist_t* allocator, u64 size) {
    mutex_lock(allocator->mutex);
    _SASSERT(allocator->memory, "Freelist has not been initialized or was not assigned memory.");
    size = smax(size, 32);

    u32 padding = 16 - (size % 16);
    if (padding) {
    SDEBUG("Padding 0x%x bytes: was 0x%x, now 0x%x bytes", padding, size, size + padding);
        size += padding;
    }

    freelist_explicit_t* explicit = allocator->first_free_block;
    while (explicit) {
        freelist_block_t* block = (void*)explicit - sizeof(freelist_block_t);

        // Check if block can contain allocation
        if (size > block->size) {
            explicit = explicit->next;
            continue;
        }

        // It can
        // Split the block into the allocation and a new block
        const u32 remaining_size = block->size - size;

        freelist_block_t* block_end = ((void*)block) + sizeof(freelist_block_t) + block->size;
        _SASSERT(block->allocated == FREELIST_FREE_MAGIC, "Allocating already allocated block.");
        _SASSERT(block_end->allocated == FREELIST_FREE_MAGIC, "Allocating already allocated block.");

        const b8 split_block = sizeof(freelist_block_t) * 2 < remaining_size;
        if (split_block) {
            freelist_block_t* new_block = ((void*)block) + sizeof(freelist_block_t) * 2 + size;
            freelist_explicit_t* new_explicit = (void*)new_block + sizeof(freelist_block_t);
            new_block->size = remaining_size - sizeof(freelist_block_t) * 2;
            new_block->allocated = FREELIST_FREE_MAGIC;

            block_end->size = new_block->size;
            block_end->allocated = FREELIST_FREE_MAGIC;
            block_end = ((void*)block) + sizeof(freelist_block_t) + size;


            new_explicit->next = explicit->next;
            new_explicit->previous = explicit->previous;

            _SASSERT((void*)new_explicit >= allocator->memory && (void*)new_explicit < allocator->memory + allocator->capacity, "Creating new explicit pointer at invalid address.");
            _SASSERT(((void*)new_explicit->next >= allocator->memory && (void*)new_explicit->next < allocator->memory + allocator->capacity) || new_explicit->next == 0, "Setting new explicit pointer to invalid address.");
            _SASSERT(((void*)new_explicit->previous >= allocator->memory && (void*)new_explicit->previous < allocator->memory + allocator->capacity) || new_explicit->previous == 0, "Setting new explicit pointer to invalid address.");

            if (explicit->next) {
                explicit->next->previous = new_explicit;
            }
            if (explicit->previous) {
                explicit->previous->next = new_explicit;
            }

            // NOTE: Following block is equivalent and is marginally faster
            if (explicit == allocator->first_free_block) {
                allocator->first_free_block = new_explicit;
            }

            _SASSERT(allocator->first_free_block->previous == NULL, "Allocation: First explicit cannot have previous.");
            // const u8 replace_first_block = explicit == allocator->first_free_block;
            // allocator->first_free_block = (void*)((size_t)allocator->first_free_block * (!replace_first_block)
            //     + (size_t)new_explicit * replace_first_block);
        } else {
            if (explicit->next) {
                explicit->next->previous = explicit->previous;
            }
            if (explicit->previous) {
                explicit->previous->next = explicit->next;
            } else {
                allocator->first_free_block = explicit->next;
            }

            size = block->size;
            block_end = (void*)block + block->size + sizeof(freelist_block_t);
            _SASSERT(allocator->first_free_block->previous == NULL, "Allocation else: First explicit cannot have previous.");
            // block_end = (void*)block + sizeof(freelist_block_t) + block->size;
        }

        // Create a new allocation
        block_end->size      = size;
        block_end->allocated = FREELIST_ALLOCATED_MAGIC;

        block->size          = size;
        block->allocated     = FREELIST_ALLOCATED_MAGIC;

        mutex_unlock(allocator->mutex);
        freelist_check_health(allocator);
        return ((void*)block) + sizeof(freelist_block_t);
    }

    mutex_unlock(allocator->mutex);
#ifdef SPARK_DEBUG
    explicit = allocator->first_free_block;
    SERROR("Freelist failed to allocate 0x%x bytes.", size);
    while (explicit) {
        freelist_block_t* block = (void*)explicit - sizeof(freelist_block_t);
        SERROR("\tBlock Size: 0x%x", block->size);
        explicit = explicit->next;
    }
    freelist_check_health(allocator);
#endif
    SCRITICAL("Freelist failed to allocate block.");
    return NULL;
}

void freelist_free(freelist_t* allocator, void* address) {
    mutex_lock(allocator->mutex);
    _SASSERT(address != NULL, "Freelist cannot free null address.");
    _SASSERT(address >= allocator->memory && address <= allocator->memory + allocator->capacity, "Cannot free address not owned by freelist. %p <= %p <= %p", allocator->memory, address, allocator->memory + allocator->capacity);

    // get allocation
    freelist_block_t* block = address - sizeof(freelist_block_t);
    freelist_block_t* block_header = address + block->size;

    _SASSERT(block_header->size > 0 && block_header->size != INVALID_ID, "Invalid block header size. 0x%x", block->size);
    _SASSERT(block->size > 0 && block->size != INVALID_ID, "Invalid block size. 0x%x", block->size);
    _SASSERT(block->allocated == FREELIST_ALLOCATED_MAGIC, "Freelist double-free detected. Cannot free unallocated block.");

    // Convert allocation to block
    enum coalesce_state {
        coalesce_state_default = 0,
        coalesce_state_previous = 1,
        coalesce_state_next = 2,
        coalesce_state_all = 3,
    };

    freelist_explicit_t* new_explicit = address;
    new_explicit->next = NULL;
    new_explicit->previous = NULL;

    freelist_block_t* previous_block = address - sizeof(freelist_block_t) * 2;
    freelist_block_t* next_block = address + block->size + sizeof(freelist_block_t);

    freelist_explicit_t* previous_explicit = (void*)previous_block - previous_block->size;
    freelist_explicit_t* next_explicit = (void*)next_block + sizeof(freelist_block_t);

    enum coalesce_state state = 0;
    if (previous_block->allocated == FREELIST_FREE_MAGIC) {
        state += coalesce_state_previous;
    }
    if (next_block->allocated == FREELIST_FREE_MAGIC) {
        state += coalesce_state_next;
    }

    _SASSERT((void*)next_explicit >= allocator->memory && (void*)next_explicit < allocator->memory + allocator->capacity, "Creating new explicit pointer at invalid address.");
    _SASSERT(((void*)next_explicit->next >= allocator->memory && (void*)next_explicit->next < allocator->memory + allocator->capacity) || next_explicit->next == 0 || next_block->allocated, "Setting new explicit pointer to invalid address.");
    _SASSERT(((void*)next_explicit->previous >= allocator->memory && (void*)next_explicit->previous < allocator->memory + allocator->capacity) || next_explicit->previous == 0 || next_block->allocated, "Setting new explicit pointer to invalid address.");

    _SASSERT((void*)previous_explicit >= allocator->memory && (void*)previous_explicit < allocator->memory + allocator->capacity, "Creating new explicit pointer at invalid address.");
    _SASSERT(((void*)previous_explicit->next >= allocator->memory && (void*)previous_explicit->next < allocator->memory + allocator->capacity) || previous_explicit->next == 0 || previous_block->allocated, "Setting new explicit pointer to invalid address.");
    _SASSERT(((void*)previous_explicit->previous >= allocator->memory && (void*)previous_explicit->previous < allocator->memory + allocator->capacity) || previous_explicit->previous == 0 || previous_block->allocated, "Setting new explicit pointer to invalid address.");

    switch (state) {
        case coalesce_state_default:
            block->allocated = FREELIST_FREE_MAGIC;
            block_header->allocated = FREELIST_FREE_MAGIC;

            new_explicit->next = allocator->first_free_block;
            new_explicit->previous = NULL;
            if (new_explicit->next) {
                new_explicit->next->previous = new_explicit;
            }
            allocator->first_free_block = new_explicit;
            mutex_unlock(allocator->mutex);
            freelist_check_health(allocator);
            return;
        case coalesce_state_previous:
            previous_block = (void*)previous_block - previous_block->size - sizeof(freelist_block_t);
            previous_block->size += block->size + sizeof(freelist_block_t) * 2;

            block = previous_block;

            block_header->size = block->size;
            block_header->allocated = FREELIST_FREE_MAGIC;

            mutex_unlock(allocator->mutex);
            freelist_check_health(allocator);
            return;
        case coalesce_state_next:
            block->allocated = FREELIST_FREE_MAGIC;
            block->size += next_block->size + sizeof(freelist_block_t) * 2;
            block_header = (void*)block_header + next_block->size + sizeof(freelist_block_t) * 2;
            block_header->size = block->size;

            if (next_explicit->previous) {
                next_explicit->previous->next = new_explicit;
            } else {
                allocator->first_free_block = new_explicit;
            }
            if (next_explicit->next) {
                next_explicit->next->previous = new_explicit;
            }
            new_explicit->next = next_explicit->next;
            new_explicit->previous = next_explicit->previous;

            mutex_unlock(allocator->mutex);
            freelist_check_health(allocator);
            return;
        case coalesce_state_all:
            // NOTE: DO NOT TOUCH THIS. IT IS JANKY AND HELD TOGETHER WITH PRAYER AND DUCT TAPE
            previous_block = (void*)previous_block - previous_block->size - sizeof(freelist_block_t);
            previous_block->size += block->size + next_block->size + sizeof(freelist_block_t) * 4;

            block = previous_block;

            block_header = (void*)block + block->size + sizeof(freelist_block_t);
            block_header->size = block->size;

            if (previous_explicit->next) {
                previous_explicit->next->previous = previous_explicit->previous;
            }
            if (previous_explicit->previous) {
                previous_explicit->previous->next = previous_explicit->next;
            } 

            if (next_explicit->next) {
                next_explicit->next->previous = next_explicit->previous;
            }
            if (next_explicit->previous) {
                next_explicit->previous->next = next_explicit->next;
            } 

            new_explicit = previous_explicit;
            new_explicit->previous = NULL;

            if (next_explicit != allocator->first_free_block && previous_explicit != allocator->first_free_block) {
                new_explicit->next = allocator->first_free_block;
            } else if (previous_explicit->next != next_explicit) {
                new_explicit->next = allocator->first_free_block->next;
            } else if (next_explicit->next != previous_explicit) {
                new_explicit->next = next_explicit->next;
            } else {
                new_explicit->next = NULL;
            }
            allocator->first_free_block = new_explicit;
            if (new_explicit->next) {
                new_explicit->next->previous = new_explicit;
            }

            mutex_unlock(allocator->mutex);
            freelist_check_health(allocator);
            return;
    }


    _SASSERT((void*)new_explicit >= allocator->memory && (void*)new_explicit < allocator->memory + allocator->capacity, "Creating new explicit pointer at invalid address.");
    _SASSERT(((void*)new_explicit->next >= allocator->memory && (void*)new_explicit->next < allocator->memory + allocator->capacity) || new_explicit->next == 0, "Setting new explicit pointer to invalid address.");
    _SASSERT(((void*)new_explicit->previous >= allocator->memory && (void*)new_explicit->previous < allocator->memory + allocator->capacity) || new_explicit->previous == 0, "Setting new explicit pointer to invalid address.");

    _SASSERT((void*)next_explicit >= allocator->memory && (void*)next_explicit < allocator->memory + allocator->capacity, "Creating new explicit pointer at invalid address.");
    _SASSERT(((void*)next_explicit->next >= allocator->memory && (void*)next_explicit->next < allocator->memory + allocator->capacity) || next_explicit->next == 0 || next_block->allocated, "Setting new explicit pointer to invalid address.");
    _SASSERT(((void*)next_explicit->previous >= allocator->memory && (void*)next_explicit->previous < allocator->memory + allocator->capacity) || next_explicit->previous == 0 || next_block->allocated, "Setting new explicit pointer to invalid address.");

    _SASSERT((void*)previous_explicit >= allocator->memory && (void*)previous_explicit < allocator->memory + allocator->capacity, "Creating new explicit pointer at invalid address.");
    _SASSERT(((void*)previous_explicit->next >= allocator->memory && (void*)previous_explicit->next < allocator->memory + allocator->capacity) || previous_explicit->next == 0 || previous_block->allocated, "Setting new explicit pointer to invalid address.");
    _SASSERT(((void*)previous_explicit->previous >= allocator->memory && (void*)previous_explicit->previous < allocator->memory + allocator->capacity) || previous_explicit->previous == 0 || previous_block->allocated, "Setting new explicit pointer to invalid address.");


    new_explicit->previous                = NULL;
    new_explicit->next                    = allocator->first_free_block;
    if (allocator->first_free_block) {
        allocator->first_free_block->previous = new_explicit;
    }
    allocator->first_free_block           = new_explicit;
    mutex_unlock(allocator->mutex);
    freelist_check_health(allocator);
}

#ifdef SPARK_DEBUG
void freelist_check_health(freelist_t* allocator) {
    mutex_lock(allocator->mutex);
    // u64 allocated = 0;
    u64 free_all = 0;
    u64 free_explicit = 0;
    // u64 capacity = allocator->capacity;
    // u64 total_block_count = 0;
    // u64 allocated_block_count = 0;
    u64 free_block_count = 0;

    freelist_block_t* block = allocator->memory + sizeof(freelist_block_t);
    static char block_buffer[8192] = {};
    u32 block_buffer_len = 0;
    while (block) {
        freelist_block_t* block_end = (void*)block + sizeof(freelist_block_t) + block->size;

        SASSERT(block_end->allocated == block->allocated, "Block header / footer allocated do not match. 0x%x != 0x%x", block->allocated, block_end->allocated);
        SASSERT(block_end->size == block->size, "Block header / footer size do not match. 0x%x != 0x%x", block->size, block_end->size);

        u32 offset = (void*)block - allocator->memory + sizeof(freelist_block_t);
        if (block->allocated == FREELIST_ALLOCATED_MAGIC) {
            // allocated += block->size;
            // allocated_block_count++;
            block_buffer_len += sprintf(block_buffer + block_buffer_len, "| A (0x%x) 0x%x ", offset, block->size);

        } else { 
            free_all += block->size;
            free_block_count++;
            block_buffer_len += sprintf(block_buffer + block_buffer_len, "| F (0x%x) 0x%x ", offset, block->size);

            s32 explicit_index = 0;
            b8 found_explicit = false;
            freelist_explicit_t* explicit = allocator->first_free_block;
            while (explicit) {
                if (explicit == (void*)block + 8) {
                    found_explicit = true;
                    break;
                }
                explicit_index++;

                explicit = explicit->next;
            }

            if (!found_explicit) {
                explicit_index = -1;
            }

            block_buffer_len += sprintf(block_buffer + block_buffer_len, "%d", explicit_index);
        }

        block = (void*)block + block->size + sizeof(freelist_block_t) * 2;
        // total_block_count++;
        if (block->size <= 0) {
            break;
        }

    }


    freelist_explicit_t* explicit = allocator->first_free_block;
    u32 explicit_block_count = 0;
    int i = 0;
    while (explicit) {
        explicit_block_count++;
        SASSERT((void*)explicit >= allocator->memory && (void*)explicit < allocator->memory + allocator->capacity, "Creating new explicit pointer at invalid address.");
        SASSERT(((void*)explicit->next >= allocator->memory && (void*)explicit->next < allocator->memory + allocator->capacity) || explicit->next == 0, "Setting new explicit pointer to invalid address.");
        SASSERT(((void*)explicit->previous >= allocator->memory && (void*)explicit->previous < allocator->memory + allocator->capacity) || explicit->previous == 0, "Setting new explicit pointer to invalid address.");

        SASSERT(explicit->next != (void*)INVALID_ID_U64, "Explicit freelist corrupted.");
        SASSERT(explicit->previous != (void*)INVALID_ID_U64, "Explicit freelist corrupted.");


        freelist_block_t* block = (void*)explicit - sizeof(freelist_block_t);
        SASSERT(block->allocated == FREELIST_FREE_MAGIC, "Explicit freelist block is not free."); 
        free_explicit += block->size;
        explicit = explicit->next;
        i++;
        if (i > 100000000) {
            SERROR("Recursive freelist detective.");
            break;
        }
    }

    // SDEBUG("Freelist Health:\n\t"
    //         "All Free             : 0x%lx\n\t"
    //         "Explicit Free        : 0x%lx\n\t"
    //         "Allocated            : 0x%lx\n\t"
    //         "Capacity             : 0x%lx\n\t"
    //         "Block Count          : %d\n\t"
    //         "Explicit block count : %d\n\t"
    //         "Free block count     : %d\n\t"
    //         "Allocated Block Count: %d", free_all, free_explicit, allocated, capacity, total_block_count, explicit_block_count, free_block_count, allocated_block_count);
    SASSERT(free_block_count == explicit_block_count, "Free block count and explicit block count do not match: %d != %d", free_block_count, explicit_block_count);
    SASSERT(free_all == free_explicit, "Freelist: All free block size not equal to explicit free size: 0x%x != 0x%x", free_all, free_explicit);
    SASSERT(allocator->first_free_block->previous == NULL, "First explicit cannot have previous.");

    // Validate explicit list 
    explicit = allocator->first_free_block;
    freelist_explicit_t* previous_explicit = NULL;
    while (explicit) {
        SASSERT(previous_explicit == explicit->previous, "Failed to properly create explicit linked list. %p != %p", explicit->previous, previous_explicit);
        previous_explicit = explicit;
        explicit = explicit->next;
        i++;
        if (i > 100000000) {
            SERROR("Recursive freelist detective.");
            break;
        }
    }
    mutex_unlock(allocator->mutex);
}
#endif
