#include "Spark/core/logging.h"
#include "Spark/memory/freelist.h"
#include "Spark/platform/platform.h"

#define get_first_block() ((freelist_block_t*)((void*)allocator.first_free_block - sizeof(freelist_block_t)))

void freelist_tests() {
    freelist_t allocator;
    constexpr u32 memory_size = 16 * MB;
    freelist_create(platform_allocate(memory_size, true), memory_size, &allocator);
    freelist_block_t block = *(freelist_block_t*)((void*)allocator.first_free_block - sizeof(freelist_block_t));

    // Allocate a value well under the limit
    constexpr u32 int_count = 64;
    // Simple allocation and free test
    {
        freelist_check_health(&allocator);
        constexpr u32 alloc_size = sizeof(int) * int_count;
        int* ints = freelist_allocate(&allocator, alloc_size);
        freelist_check_health(&allocator);

        for (u32 i = 0; i < int_count; i++) {
            ints[i] = i;
        }

        for (u32 i = 0; i < int_count; i++) {
            SASSERT(ints[i] == i, "This should not be possible");
        }

        freelist_free(&allocator, ints);
        freelist_check_health(&allocator);
        // New block size should be equal to the old block size due to defragmentation
        if (block.size != get_first_block()->size) {
            SERROR("Failed to defragment general allocator. Expected size of 0x%x, got 0x%x", block.size, get_first_block()->size);
        } else {
            SINFO("Freelist basic alloc / dealloc success");
        }
    }


    // Gap defragmentation test
    // This creates 4 allocations and frees them to create gaps
    // This checks if the allocator is capable of automatically defragmenting
    { 
        const u32 int_count = 8;
        int* ints[int_count];
        for (u32 i = 0; i < int_count; i++) {
            ints[i] = freelist_allocate(&allocator, sizeof(int));
        }

        freelist_check_health(&allocator);
        freelist_free(&allocator, ints[0]);
        freelist_check_health(&allocator);
        freelist_free(&allocator, ints[3]);
        freelist_check_health(&allocator);
        freelist_free(&allocator, ints[2]);
        freelist_check_health(&allocator);
        freelist_free(&allocator, ints[1]);
        freelist_check_health(&allocator);
        freelist_free(&allocator, ints[7]);
        freelist_check_health(&allocator);
        freelist_free(&allocator, ints[4]);
        freelist_check_health(&allocator);
        freelist_free(&allocator, ints[5]);
        freelist_check_health(&allocator);
        freelist_free(&allocator, ints[6]);
        freelist_check_health(&allocator);

        if (block.size == get_first_block()->size) {
            SINFO("General allocator passed gap defragmentation test");
        } else {
            SERROR("Failed to defragment general allocator. Expected size of 0x%x, got 0x%x. First Block: %p", block.size, get_first_block()->size, get_first_block());
        }
    }

    // Allocate something over the allocator size
    {
        constexpr u32 max_int_count = memory_size / sizeof(int) / 2;
        int* ints = freelist_allocate(&allocator, max_int_count * sizeof(int));

        for (u32 i = 0; i < max_int_count; i++) {
            ints[i] = i;
        }

        for (u32 i = 0; i < max_int_count; i++) {
            SASSERT(ints[i] == i, "This should not be possible");
        }
        freelist_free(&allocator, ints);
        if (block.size != get_first_block()->size) {
            SERROR("Failed to defragment general allocator. Expected size of 0x%x, got 0x%x", block.size, get_first_block()->size);
        } else {
            SINFO("Freelist chunk overflow test success.");
        }
    }

    // Random alloc dealloc
    {
        constexpr int int_count = 100;
        constexpr int iteration_count = 100000;
        int* rand_ints[int_count] = {};
        for (u32 i = 0; i < iteration_count; i++) {
            u32 index = random() % int_count;
            if (rand_ints[index]) {
                SDEBUG("Free  %d (%d)", index, i);
                freelist_free(&allocator, rand_ints[index]);
                rand_ints[index] = NULL;
                SASSERT(rand_ints[index] == NULL, "Failed to set free thing to null :(");
            } else {
                SDEBUG("Alloc %d (%d)", index, i);
                rand_ints[index] = freelist_allocate(&allocator, random() % 1024);
            }
        }

        for (u32 i = 0; i < int_count; i++) {
            if (rand_ints[i]) {
                freelist_free(&allocator, rand_ints[i]);
            }
        }

        freelist_check_health(&allocator);
        if (block.size != get_first_block()->size) {
            SERROR("Failed to defragment general allocator. Expected size of 0x%x, got 0x%x", block.size, get_first_block()->size);
        } else {
            SINFO("Freelist random deallocate success");
        }
    }

    // Overflow test
    // for (u32 i = 1; i <= 16; i++) {
    //     u32 alloc_size = 1024;
    //     const u32 max_int_count = memory_size / (alloc_size) * i;
    //     int* ints[max_int_count];
    //
    //     for (u32 i = 0; i < max_int_count; i++) {
    //         ints[i] = freelist_allocate(&allocator, alloc_size);
    //     }
    //
    //     for (u32 i = 0; i < max_int_count * 50; i++) {
    //         u32 index = random() % max_int_count;
    //         if (ints[index]) {
    //             freelist_free(&allocator, ints[index]);
    //             ints[index] = NULL;
    //         }
    //     }
    //     for (u32 i = 0; i < max_int_count; i++) {
    //         if (ints[i]) {
    //             freelist_free(&allocator, ints[i]);
    //         }
    //     }
    //
    //     if (block.size != get_first_block()->size) {
    //         SERROR("Failed to defragment general allocator. Expected size of 0x%x, got 0x%x", block.size, get_first_block()->size);
    //     } else {
    //         SINFO("Freelist overflow test success");
    //     }
    // }
}
