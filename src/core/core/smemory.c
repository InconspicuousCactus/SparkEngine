/**
 * @file
 * @brief Interfaces with platform to manage and track memory
 */

#include "Spark/core/smemory.h"
#include "Spark/containers/generic/darray_ints.h"
#include "Spark/core/logging.h"
#include "Spark/core/sstring.h"
#include "Spark/memory/dynamic_allocator.h"
#include "Spark/memory/freelist.h"
#include "Spark/platform/platform.h"
#include <execinfo.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

// Used for allocation tracking
#ifdef SPARK_DEBUG
#include "Spark/containers/darray.h"

typedef struct allocation_info {
    const char* file;
    u32 line;
    u64 size;
    memory_tag_t tag;
    void* block;
    char* backtrace;
} allocation_info_t;

darray_type(allocation_info_t, allocation_info);
darray_impl(allocation_info_t, allocation_info);

u32 allocation_count = 0;
u32 allocation_capacity = 0;
allocation_info_t* tracked_allocations = NULL;
#endif

typedef struct {
    u64 total_allocated;
    u64 tagged_allocations[MEMORY_TAG_MAX];
} memory_stats_t;

const char* memory_tag_strings[] = {
    "UNDEFINED          ",
    "ENTITY             ",
    "ECS                ",
    "SYSTEM             ",
    "STRING             ",
    "DARRAY             ",
    "ARRAY              ",
    "JOB                ",
    "TEXTURE            ",
    "MATERIAL           ",
    "SHADER             ",
    "MODEL              ",
    "GAME               ",
    "RENDERER           ",
    "LINEAR_ALLOCATOR   ",
    "MATERIAL_INSTANCE  ",
    "THREAD             ",
};

typedef struct memory_system_state {
    memory_stats_t stats;
    u64 alloc_count;
    dynamic_allocator_t allocator;
} memory_system_state_t;

static memory_system_state_t state_ptr;

static char* memory_usage_string;
static int memory_usage_string_size = 0x8000;

void initialize_memory() {
    state_ptr.alloc_count = 0;

    dynamic_allocator_create(128 * MB, &state_ptr.allocator);
    memory_usage_string = dynamic_allocator_allocate(&state_ptr.allocator, memory_usage_string_size);

#ifdef SPARK_DEBUG 
    allocation_count = 0;
    allocation_capacity = 8192;
    tracked_allocations = dynamic_allocator_allocate(&state_ptr.allocator, sizeof(allocation_info_t) * allocation_capacity);
#endif
}

/**
 * @brief Shutdown memory system
 */
void 
shutdown_memory() {
#ifdef SPARK_DEBUG
    // Ensure all allocations are removed
    if (allocation_count > 0) {
        SWARN("FAILED TO FREE ALL ALLOCATIONS: %d REMAINING.", allocation_count);
    }

    for (u32 i = 0; i < allocation_count; i++) {
        // SWARN("File: %s:%d", tracked_allocations[i].file, tracked_allocations[i].line);
        // if (tracked_allocations[i].backtrace) {
        //     SWARN("Backtrace:\n%s", tracked_allocations[i].backtrace);
        // }
    }
#endif

    SDEBUG("Memory after shutdown: %s", get_memory_usage_string());
    dynamic_allocator_destroy(&state_ptr.allocator);
}

/**
 * @brief Allocates [size] bytes of memory to type [tag] and tracks the number of bytes used.
 *
 * @param size 
 * @param tag 
 */
void*  
pvt_sallocate(u64 size, memory_tag_t tag) {
    if (size == 0) {
        SERROR("Cannot allocate zero bytes of memory");
        return NULL;
    }

    if (tag == MEMORY_TAG_UNDEFINED) {
        SWARN("Allocating %lu bytes to undefined memory tag.", size);
    }

    state_ptr.stats.total_allocated += size;
    state_ptr.stats.tagged_allocations[tag] += size;

    void* block = platform_allocate(size, true);
    platform_zero_memory(block, size);
    // void* block = dynamic_allocator_allocate(&state_ptr.allocator, size);
    return block;
}

/**
 * @brief Frees memory [block] and tracks the number of bytes freed
 *
 * @param block Block of memory to be freed
 * @param size Number of bytes that [block] contains. This is used for memory tracking
 * @param tag Type of memory that is being freed. This is used for memory tracking
 */
void   
pvt_spark_free(const void* block, u64 size, memory_tag_t tag) {
    if (tag == MEMORY_TAG_UNDEFINED) {
        SWARN("De-allocating %lu bytes to undefined memory tag.", size);
    }

    if (state_ptr.stats.tagged_allocations[tag] - size > state_ptr.stats.tagged_allocations[tag]) {
        SCRITICAL("Underflowed a memory allocation tag by freeing %lu bytes. Before %lu, After %lu - Failed to free the correct type of memory '%s'", size, state_ptr.stats.tagged_allocations[tag], state_ptr.stats.tagged_allocations[tag] - size, memory_tag_strings[tag]);
    } 

    state_ptr.stats.total_allocated -= size;
    state_ptr.stats.tagged_allocations[tag] -= size;

    // dynamic_allocator_free(&state_ptr.allocator, (void*)block);
    platform_free((void*)block, true);
}

/**
 * @brief Zeros out size bytes at address block.
 *
 * @param block target address
 * @param size number of bytes to set
 */
void* 
szero_memory(void* block, u64 size) {
    return platform_zero_memory(block, size);
}

/**
 * @brief Sets size bytes to value at address block
 *
 * @param block memory to be set
 * @param value value that each byte will be set
 * @param size number of bytes to set
 */
void* 
sset_memory(void* block, s32 value, u64 size) {
    return platform_set_memory(block, value, size);
}

/**
 * @brief Copies size bytes from src to dest
 *
 * @param dest destination pointer
 * @param src source pointer
 * @param size number of bytes to be copied
 */
void* 
scopy_memory(void* dest, const void* src, u64 size) {
    return platform_copy_memory(dest, src, size);
}

void copy_memory_usage_string(const char* buffer, const char* tag_string, u64 size, u64* offset) {
    const u64 kib = 1024;
    const u64 mib = 1024 * 1024;
    const u64 gib = 1024 * 1024 * 1024;

    const char* units[] = {
        "B  ",
        "KiB",
        "MiB",
        "GiB",
    };
    const char* unit = units[0];
    float amount = 0;
    if (size < kib) {
        unit = units[0];
        amount = size;
    } else if (size < mib) {
        unit = units[1];
        amount = size / (f32)kib;
    } else if (size < gib) {
        unit = units[2];
        amount = size / (f32)mib;
    } else {
        unit = units[3];
        amount = size / (f32)gib;
    }

    s32 length = snprintf(memory_usage_string + *offset, memory_usage_string_size, "\t\t\t%s: %.2f%s\n", tag_string, amount, unit);
    *offset += length;
}

/**
 * @brief Gives a string which returns how much memory is being used for each type.
 *
 * @return const char* owned by smemory.c
 */
const 
char* get_memory_usage_string() {

    strcpy(memory_usage_string, "System memory use (tagged):\n");
    u64 offset = strlen(memory_usage_string);
    copy_memory_usage_string(memory_usage_string, "TOTAL              ", state_ptr.stats.total_allocated, &offset);

    for (int i = 0; i < MEMORY_TAG_MAX; i++) {
        u64 size = state_ptr.stats.tagged_allocations[i];
        copy_memory_usage_string(memory_usage_string, memory_tag_strings[i], size, &offset);
    }

    memory_usage_string[offset] = 0;

    return memory_usage_string;
}

u64 get_memory_alloc_count() {
    return state_ptr.alloc_count;
}

#ifdef SPARK_DEBUG 
void* 
create_tracked_allocation(u64 size, memory_tag_t tag, const char* file, u32 line) {
    void* block = pvt_sallocate(size, tag);

    // Get backtrace string
    const u32 buffer_size = 100;
    void* buffer[100];
    char** backtrace_strings;
    u32 backtrace_pointer_count = backtrace(buffer, buffer_size);
    backtrace_strings = backtrace_symbols(buffer, backtrace_pointer_count);
    if (backtrace_strings == NULL) {
        SERROR("backtrace_symbols failed to get symbols");
    }

    // char* backtrace_string = dynamic_allocator_allocate(&state_ptr.allocator, 0x1000);
    // char* backtrace_string = platform_allocate(0x1000, MEMORY_TAG_STRING);
    // char* backtrace_string = NULL;
    // szero_memory(backtrace_string, 0x1000);
    // for (u32 i = 1, offset = 0; i < backtrace_pointer_count; i++) {
    //
    //     u32 p = 0;
    //     while(backtrace_strings[i][p] != '(' && backtrace_strings[i][p] != ' ' && backtrace_strings[i][p] != 0) {
    //         ++p;
    //     }
    //
    //     u32 len = string_length(backtrace_strings[i]);
    //     scopy_memory(backtrace_string + offset, backtrace_strings[i], len);
    //     backtrace_string[offset + len - 1] = '\n';
    //     offset += len;
    // }

    // allocation_info_t info = {
    //     .file = file,
    //     .line = line,
    //     .tag = tag,
    //     .size = size,
    //     .block = block,
    //     .backtrace = backtrace_string,
    // };
    //
    // if (tracked_allocations) {
    //     // Ensure that the darray doesnt try to allocate itself in an infinite loop by managing it here
    //     if (allocation_count >= allocation_capacity) {
    //         allocation_info_t* old_allocations = tracked_allocations;
    //         tracked_allocations = pvt_sallocate(allocation_capacity * 2 * sizeof(allocation_info_t), MEMORY_TAG_ARRAY);
    //
    //         scopy_memory(tracked_allocations, old_allocations, allocation_count * sizeof(allocation_info_t));
    //         pvt_spark_free(old_allocations, sizeof(allocation_info_t) * allocation_capacity, MEMORY_TAG_ARRAY);
    //         allocation_capacity *= 2;
    //     }
    //
    //     tracked_allocations[allocation_count++] = info;
    // }

    return block;
}

void 
free_tracked_allocation(const void* block, u32 size, memory_tag_t tag) {
    // Find the allocation by its block
    for (int i = 0; i < allocation_count; i++) {
        if (tracked_allocations[i].block == block) {
            // SASSERT(tracked_allocations[i].tag == tag,  "Trying to free allocation with different memory tag than it was initialized with.\n\t\t"
            //         "Tag: %s\n\t\tFile: %s:%d\n\t\tBacktrace:\n%s", memory_tag_strings[tracked_allocations[i].tag], tracked_allocations[i].file, tracked_allocations[i].line, tracked_allocations[i].backtrace);
            // SASSERT(tracked_allocations[i].size == size,  "Trying to free allocation with different memory size than it was initialized with.\n\t\t"
            //         "Expected: %lul\n\t\tGot: %ul\n\t\tFile: %s:%d\n\t\tBacktrace:\n%s", tracked_allocations[i].size, size, tracked_allocations[i].file, tracked_allocations[i].line, tracked_allocations[i].backtrace);

            if (tracked_allocations[i].backtrace) {
                // dynamic_allocator_free(&state_ptr.allocator, tracked_allocations[i].backtrace);
                platform_free(tracked_allocations[i].backtrace, true);
                tracked_allocations[i].backtrace = NULL;
            }

            for (u32 j = i; j < allocation_count - 1; j++) {
                tracked_allocations[j] = tracked_allocations[j + 1];
            }
                // scopy_memory(&tracked_allocations[i], &tracked_allocations[i + 1], (allocation_count - i) * sizeof(allocation_info_t));
            allocation_count--;
        }
    }

    pvt_spark_free(block, size, tag);
}
#endif
