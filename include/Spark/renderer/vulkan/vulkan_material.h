#pragma once

#include "Spark/renderer/material.h"
#include "Spark/containers/darray.h"
#include "Spark/renderer/vulkan/vulkan_buffer.h"
#include <vulkan/vulkan_core.h>

struct vulkan_context;
struct vulkan_command_buffer;

#define VULKAN_MATERIAL_MAX_SETS 6
typedef struct vulkan_material {
    VkDescriptorSet sets[VULKAN_MATERIAL_MAX_SETS];
    u8 set_count;
    u8 first_set;
    vulkan_buffer_t* storage_buffer;
} vulkan_material_t;

darray_header(vulkan_material_t, vulkan_material);

void vulkan_material_create(struct vulkan_context* context, vulkan_material_t* out_material);
void vulkan_material_destroy(struct vulkan_context* context, vulkan_material_t* material);

void vulkan_material_bind(struct vulkan_context* context, struct vulkan_command_buffer* command_buffer, material_t* material);
void vulkan_material_update_buffer(struct vulkan_context* context, void* data, u32 size, u32 offset, vulkan_material_t* material);
