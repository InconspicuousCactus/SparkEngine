#pragma once

#include "Spark/math/mat4.h"
#include "Spark/math/math_types.h"
#include "Spark/memory/block_allocator.h"
#include "Spark/memory/freelist.h"
#include "Spark/renderer/renderer_types.h"
#include "Spark/renderer/renderpasses.h"
#include "Spark/renderer/vulkan/vulkan_buffer.h"
#include "Spark/renderer/vulkan/vulkan_command_buffer.h"
#include "Spark/renderer/vulkan/vulkan_material.h"
#include "Spark/renderer/vulkan/vulkan_mesh.h"
#include "Spark/renderer/vulkan/vulkan_physical_device.h"
#include "Spark/renderer/vulkan/vulkan_queue.h"
#include "Spark/renderer/vulkan/vulkan_renderpass.h"
#include "Spark/renderer/vulkan/vulkan_shader.h"
#include "Spark/renderer/vulkan/vulkan_swapchain.h"
#include "Spark/renderer/vulkan/vulkan_texture.h"
#include <vulkan/vulkan_core.h>

#define VULKAN_CONTEXT_VERTEX_BUFFER_SIZE (1024* 1024 * 1024)
#define VULKAN_CONTEXT_INDEX_BUFFER_SIZE (sizeof(u32) * 128 * 1024 * 1024)
#define VULKAN_CONTEXT_MAX_STORAGE_BUFFERS 64
#define VULKAN_CONTEXT_MAX_UNIFORM_BUFFERS 64
#define VULKAN_CONTEXT_MAX_SAMPLERS 64
#define VULKAN_CONTEXT_MAX_DESCRIPTOR_SETS 64
#define VULKAN_CONTEXT_INSTANCE_BUFFER_COUNT 8192 * 16
#define VULKAN_INDIRECT_BUFFER_SIZE (1024 * 32 * sizeof(VkDrawIndexedIndirectCommand))


typedef struct indirect_draw_info {
    u32 command_index;
    builtin_renderpass_t shader_type;
    material_t* material;
} indirect_draw_info_t;

darray_header(VkDrawIndexedIndirectCommand, VkDrawIndexedIndirectCommand);
darray_header(indirect_draw_info_t, indirect_draw_info);

typedef struct indirect_info {
    darray_VkDrawIndexedIndirectCommand_t commands;
    darray_indirect_draw_info_t draws;
    vulkan_buffer_t buffer;
} vulkan_indirect_render_info_t;

typedef struct vulkan_context {
    VkInstance instance;
    VkAllocationCallbacks* allocator;

    u32 screen_width;
    u32 screen_height;

    vulkan_physical_device_t physical_device;
    VkDevice logical_device;

    VkSurfaceKHR surface;

    vulkan_queue_t graphics_queue;
    vulkan_queue_t present_queue;
    vulkan_queue_t compute_queue;
    vulkan_queue_t transfer_queue;


    u32 current_frame;
    u32 image_count;
    u32 image_index;
    vulkan_swapchain_t swapchain;

    vulkan_command_buffer_t copy_command_buffer;
    vulkan_command_buffer_t copy_to_graphics_command_buffer;
    vulkan_command_buffer_t command_buffers[SWAPCHAIN_MAX_IMAGE_COUNT];
    VkSemaphore present_complete_semaphores[SWAPCHAIN_MAX_IMAGE_COUNT];
    VkSemaphore render_complete_semaphores[SWAPCHAIN_MAX_IMAGE_COUNT];
    VkFence in_flight_fences[SWAPCHAIN_MAX_IMAGE_COUNT];

    vulkan_renderpass_t renderpasses[BUILTIN_RENDERPASS_ENUM_MAX];
    darray_mat4_t local_instance_buffer;

    // Descriptor pools
    VkDescriptorPool descriptor_pool;

    // Buffers
    vulkan_buffer_t uniform_buffer_skybox;
    vulkan_buffer_t uniform_buffer_3d;
    vulkan_buffer_t uniform_buffer_ui;
    vulkan_buffer_t instance_buffer;
    
    vulkan_indirect_render_info_t indirect_draw_infos[SWAPCHAIN_MAX_IMAGE_COUNT];

    vulkan_buffer_t vertex_buffer;
    freelist_t vertex_buffer_freelist;

    vulkan_buffer_t index_buffer;
    freelist_t index_buffer_freelist;


    u64 text_buffer_size;
    const char* shader_text_buffer;

    block_allocator_t shader_buffer_allocator;

    // Data
    vulkan_image_t    default_texture;
    vulkan_shader_t*   default_shader;
    vulkan_material_t* default_material;

    renderer_defaults_t default_types;

    block_allocator_t   shader_allocator;
    block_allocator_t   material_allocator;
    block_allocator_t   image_allocator;
    block_allocator_t   mesh_allocator;

#ifdef SPARK_DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
} vulkan_context_t;
