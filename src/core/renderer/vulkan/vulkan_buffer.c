#include "Spark/renderer/vulkan/vulkan_buffer.h"
#include "Spark/core/smemory.h"
#include "Spark/renderer/vulkan/vulkan_command_buffer.h"
#include "Spark/renderer/vulkan/vulkan_context.h"
#include "Spark/renderer/vulkan/vulkan_queue.h"
#include "Spark/renderer/vulkan/vulkan_texture.h"
#include "Spark/renderer/vulkan/vulkan_utils.h"
#include <vulkan/vulkan_core.h>

void vulkan_buffer_create(struct vulkan_context* context, u32 size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, vulkan_queue_t* queue, vulkan_buffer_t* out_buffer) {
    VkBufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // TODO: Multiple queues being used
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queue->family_index,
    };

    VK_CHECK(vkCreateBuffer(context->logical_device, &create_info, context->allocator, &out_buffer->handle));

    // Get memory requirements
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(context->logical_device, out_buffer->handle, &memory_requirements);

    out_buffer->allocation_size = memory_requirements.size;

    // Get memory type index
    u32 memory_index = vulkan_get_memory_index(context, memory_requirements.memoryTypeBits, properties);

    // Alocate memory
    VkMemoryAllocateInfo allocation_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize= memory_requirements.size,
        .memoryTypeIndex = memory_index,
    };

    VK_CHECK(vkAllocateMemory(context->logical_device, &allocation_info, context->allocator, &out_buffer->memory));

    // Bind memory
    VK_CHECK(vkBindBufferMemory(context->logical_device, out_buffer->handle, out_buffer->memory, 0));

    // Set internal type
    // TODO: Find a way to not use if else blocks to set type
    if (usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
        out_buffer->type = VULKAN_BUFFER_TYPE_STORAGE;
    } else if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
        out_buffer->type = VULKAN_BUFFER_TYPE_UNIFORM;
    }
}

void vulkan_buffer_destroy(struct vulkan_context* context, vulkan_buffer_t* buffer) {
    vkFreeMemory(context->logical_device, buffer->memory, context->allocator);
    vkDestroyBuffer(context->logical_device, buffer->handle, context->allocator);
}

void vulkan_buffer_copy(struct vulkan_context* context, vulkan_buffer_t* dest, vulkan_buffer_t* source, u32 size) {
    vulkan_command_buffer_record(&context->copy_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkBufferCopy copy_info = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };

    vkCmdCopyBuffer(context->copy_command_buffer.handle, source->handle, dest->handle, 1, &copy_info);
    vulkan_command_buffer_end_recording(&context->copy_command_buffer);

    vulkan_queue_submit_sync(&context->transfer_queue, &context->copy_command_buffer);
    vulkan_queue_wait_idle(&context->transfer_queue);
}

void vulkan_buffer_copy_to_image(struct vulkan_context* context, vulkan_image_t* dest, vulkan_buffer_t* source, VkImageLayout image_layout, u32 width, u32 height) {
    vulkan_command_buffer_record(&context->copy_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkBufferImageCopy copy_info = {
        .imageSubresource = {
            .baseArrayLayer = 0,
            .layerCount = 1,
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
        },
        .imageOffset = { .x = 0, .y = 0 },
        .imageExtent = {
            .depth = 1,
            .width = width,
            .height = height,
        },
    };

    vkCmdCopyBufferToImage(context->copy_command_buffer.handle, source->handle, dest->handle, image_layout, 1, &copy_info);
    vulkan_command_buffer_end_recording(&context->copy_command_buffer);

    vulkan_queue_submit_sync(&context->transfer_queue, &context->copy_command_buffer);
    vulkan_queue_wait_idle(&context->transfer_queue);
}

void vulkan_buffer_set_data(struct vulkan_context* context, vulkan_buffer_t* buffer, void* data, u32 size) {
    // Create staging buffer
    vulkan_buffer_t staging_buffer;
    vulkan_buffer_create(context, size, 
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            &context->transfer_queue,
                            &staging_buffer);

    // Map memory
    void* memory = NULL;
    VK_CHECK(vkMapMemory(context->logical_device, staging_buffer.memory, 0, staging_buffer.allocation_size, 0, &memory));

    // Copy data
    scopy_memory(memory, data, size);

    // Unmap memory
    vkUnmapMemory(context->logical_device, staging_buffer.memory);

    // Set data
    vulkan_buffer_copy(context, buffer, &staging_buffer, size);

    vulkan_buffer_destroy(context, &staging_buffer);
}


void vulkan_buffer_update(struct vulkan_context* context, vulkan_buffer_t* buffer, const void* data, u32 size, u32 offset) {
    if (size <= 0) {
        return;
    }
    void* memory = NULL;
    VK_CHECK(vkMapMemory(context->logical_device, buffer->memory, offset, size, 0, &memory));
    scopy_memory(memory, data, size);
    vkUnmapMemory(context->logical_device, buffer->memory);
}


void vulkan_buffer_create_descriptor_write(vulkan_buffer_t* buffer, 
        u32 binding_index, 
        VkDescriptorSet descriptor_set, 
        VkDescriptorType type, 
        VkDescriptorBufferInfo* buffer_info, 
        VkWriteDescriptorSet* write) {
    buffer_info->buffer = buffer->handle;
    buffer_info->offset = 0;
    buffer_info->range = VK_WHOLE_SIZE;

    write->sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write->dstSet          = descriptor_set;
    write->dstBinding      = binding_index;
    write->dstArrayElement = 0;
    write->descriptorCount = 1;
    write->pBufferInfo     = buffer_info;
    write->descriptorType  = type;
}

