
#include "Spark/memory/block_allocator.h"
#include "Spark/core/smemory.h"
#include "Spark/math/smath.h"

void block_allocator_create(u32 block_count, u32 block_size, block_allocator_t* out_allocator) {
    const u32 total_block_size = smax(block_size, sizeof(block_allocator_block_t));
    out_allocator->buffer = sallocate(total_block_size * block_count, MEMORY_TAG_ALLOCATOR);
    out_allocator->first_block = out_allocator->buffer;
    out_allocator->block_count = block_count;
    out_allocator->block_size = block_size;

    for (u32 i = 0; i < block_count - 1; i++) {
        u32 offset = i * total_block_size;
        block_allocator_block_t* block = (block_allocator_block_t*)(out_allocator->buffer + offset);
        block->next = out_allocator->buffer + (i + 1) * total_block_size;
    }
}

void block_allocator_destroy(block_allocator_t* allocator) {
    sfree(allocator->buffer, allocator->block_count * allocator->block_size, MEMORY_TAG_ALLOCATOR);
}

void* block_allocator_allocate(block_allocator_t* allocator) {
    if (allocator->first_block == NULL) {
        SCRITICAL("Cannot allocate, no blocks remaining.");
        return NULL;
    }

    block_allocator_block_t* block = allocator->first_block;
    block->magic = 0;
    allocator->first_block = block->next;
    // szero_memory(block, sizeof(block_allocator_block_t));

    return block;
}

void block_allocator_free(block_allocator_t* allocator, void* block) {
    SASSERT(block >= allocator->buffer && block < allocator->buffer + allocator->block_count * allocator->block_size, 
            "Cannot free block %p: Outsize of memory range (%p - %p)", 
            block, 
            allocator->buffer, 
            allocator->buffer + allocator->block_size * allocator->block_count);

    block_allocator_block_t* _block = block;
    _block->magic = BLOCK_ALLOCATOR_MAGIC;
    _block->next = allocator->first_block;
    allocator->first_block = _block->next;
}

void block_allocator_zero_unused_blocks(block_allocator_t* allocator) {
    block_allocator_block_t* block = allocator->first_block;
    while (block) {
        block_allocator_block_t* next_block = block->next;
        szero_memory(block, allocator->block_size);
        block = next_block;
    }
}
