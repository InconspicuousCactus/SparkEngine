
#include "Spark/memory/block_allocator.h"
#include "Spark/core/smemory.h"
void block_allocator_create(u32 block_count, u32 block_size, block_allocator_t* out_allocator) {
    const u32 total_block_size = block_size * sizeof(block_allocator_block_t);
    out_allocator->buffer = sallocate(total_block_size * block_count, MEMORY_TAG_ALLOCATOR);
    out_allocator->blocks = out_allocator->buffer;
    out_allocator->buffer_size = total_block_size * block_count;

    for (u32 i = 0; i < block_count - 1; i++) {
        u32 offset = i * total_block_size;
        block_allocator_block_t* block = (block_allocator_block_t*)(out_allocator->buffer + offset);
        block->next = out_allocator->buffer + (i + 1) * total_block_size;
    }
}

void block_allocator_destroy(u32 block_count, u32 block_size, block_allocator_t* out_allocator) {
    const u32 total_block_size = block_size * sizeof(block_allocator_block_t);
    sfree(out_allocator->buffer, total_block_size * block_count, MEMORY_TAG_ALLOCATOR);
}

void* block_allocator_allocate(block_allocator_t* allocator) {
    if (allocator->blocks == NULL) {
        SCRITICAL("Cannot allocate, no blocks remaining.");
        return NULL;
    }

    block_allocator_block_t* block = allocator->blocks;
    block_allocator_block_t* allocation = block  + 1;
    allocator->blocks = block->next;
    // szero_memory(block, sizeof(block_allocator_block_t));

    return allocation;
}

void block_allocator_free(block_allocator_t* allocator, void* block) {
    SASSERT(block >= allocator->buffer && block < allocator->buffer + allocator->buffer_size, 
            "Cannot free block %p: Outsize of memory range (%p - %p)", 
            block, 
            allocator->buffer, 
            allocator->buffer + allocator->buffer_size);

    block_allocator_block_t* _block = block - sizeof(block_allocator_block_t);
    _block->next = allocator->blocks;
    allocator->blocks = _block->next;
}

