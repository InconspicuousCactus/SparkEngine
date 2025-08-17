#pragma once

#include "Spark/memory/freelist.h"
#include "Spark/renderer/vulkan/vulkan_buffer.h"

typedef struct vulkan_dynamic_buffer {
    vulkan_buffer_t internal_buffer;
    freelist_t freelist;
} vulkan_dynamic_buffer_t;

void vulkan_dynamic_buffer_create(struct vulkan_context* context, 
                                    u32 size, 
                                    VkBufferUsageFlags usage, 
                                    VkMemoryPropertyFlags properties, 
                                    struct vulkan_queue* queue,  
                                    vulkan_dynamic_buffer_t* out_buffer);
void vulkan_dynamic_buffer_destroy(vulkan_dynamic_buffer_t* buffer);

void* vulkan_dynamic_buffer_allocate(vulkan_dynamic_buffer_t* buffer, u32 size);
void vulkan_dynamic_buffer_free(vulkan_dynamic_buffer_t* buffer, void* data);
