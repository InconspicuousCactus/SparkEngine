#include "Spark/renderer/vulkan/vulkan_dynamic_buffer.h"
#include "Spark/renderer/vulkan/vulkan_buffer.h"

void vulkan_dynamic_buffer_create(struct vulkan_context* context, 
                                    u32 size, 
                                    VkBufferUsageFlags usage, 
                                    VkMemoryPropertyFlags properties, 
                                    struct vulkan_queue* queue,  
                                    vulkan_dynamic_buffer_t* out_buffer) {
    // vulkan_buffer_create(context, size, usage, properties, queue, &out_buffer->internal_buffer);
    // freelist
}
void vulkan_dynamic_buffer_destroy(vulkan_dynamic_buffer_t* buffer);

void* vulkan_dynamic_buffer_allocate(vulkan_dynamic_buffer_t* buffer, u32 size);
void vulkan_dynamic_buffer_free(vulkan_dynamic_buffer_t* buffer, void* data);
