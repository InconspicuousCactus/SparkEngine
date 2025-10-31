#pragma once

#define BLOCK_ALLOCATOR_MAGIC 0xAEFF9025
typedef struct block_allocator_block {
    u32 magic;
    struct block_allocator_block* next;
} block_allocator_block_t;

typedef struct block_allocator {
    void* buffer;
    block_allocator_block_t* first_block;
    u32 block_count;
    u32 block_size;
} block_allocator_t;

void block_allocator_create(u32 block_count, u32 block_size, block_allocator_t* out_allocator);
void block_allocator_destroy(block_allocator_t* allocator);

void* block_allocator_allocate(block_allocator_t* allocator);
void block_allocator_free(block_allocator_t* allocator, void* block);
void block_allocator_zero_unused_blocks(block_allocator_t* allocator);

#define block_allocator_iterate(allocator, arg_name, function) \
{ \
    block_allocator_block_t* block = (allocator)->buffer; \
    for (u32 i = 0; i < (allocator)->block_count; i++) { \
        void* arg_name = block; \
        if (block->magic == BLOCK_ALLOCATOR_MAGIC) { \
            function \
        } \
        block += (allocator)->block_size; \
    } \
}
