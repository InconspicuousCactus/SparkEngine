#include "Spark/renderer/vulkan/vulkan_backend.h"
#include "Spark/containers/darray.h"
#include "Spark/core/smemory.h"
#include "Spark/core/sstring.h"
#include "Spark/defines.h"
#include "Spark/math/mat4.h"
#include "Spark/math/quat.h"
#include "Spark/memory/block_allocator.h"
#include "Spark/renderer/material.h"
#include "Spark/renderer/mesh.h"
#include "Spark/renderer/renderer_types.h"
#include "Spark/renderer/renderpasses.h"
#include "Spark/renderer/shader.h"
#include "Spark/renderer/texture.h"
#include "Spark/renderer/vulkan/vulkan_buffer.h"
#include "Spark/renderer/vulkan/vulkan_command_buffer.h"
#include "Spark/renderer/vulkan/vulkan_context.h"
#include "Spark/renderer/vulkan/vulkan_fence.h"
#include "Spark/renderer/vulkan/vulkan_material.h"
#include "Spark/renderer/vulkan/vulkan_mesh.h"
#include "Spark/renderer/vulkan/vulkan_physical_device.h"
#include "Spark/renderer/vulkan/vulkan_platform.h"
#include "Spark/renderer/vulkan/vulkan_queue.h"
#include "Spark/renderer/vulkan/vulkan_renderpass.h"
#include "Spark/renderer/vulkan/vulkan_semaphor.h"
#include "Spark/renderer/vulkan/vulkan_shader.h"
#include "Spark/renderer/vulkan/vulkan_swapchain.h"
#include "Spark/renderer/vulkan/vulkan_texture.h"
#include "Spark/renderer/vulkan/vulkan_utils.h"

#include "Spark/memory/linear_allocator.h"
#include "Spark/resources/loaders/material_loader.h"
#include "Spark/resources/loaders/shader_loader.h"
#include "Spark/resources/resource_loader.h"
#include "Spark/resources/resource_types.h"
#include "Spark/resources/resource_functions.h"
#include <vulkan/vulkan_core.h>

vulkan_context_t* context = NULL;

// Private functions
void create_vulkan_instance(const char* application_name);
void get_physical_device();
u32 get_physical_device_score(VkPhysicalDevice device);
void create_logical_device();
void get_depth_format();
void create_command_buffers();
void create_renderpasses();
void create_shader_module();
void create_descriptor_pools();

void create_vulkan_debug_callback();

VkBool32 vulkan_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data);

/**
 * @brief Returns a score for the best physical device. Higher is better
 * @return 
 */
// End Private functions
b8 vulkan_renderer_initialize(const char* application_name, struct platform_state* plat_state, linear_allocator_t* allocator) {
    STRACE("Creating vulkan renderer");

    // Create context
    context = linear_allocator_allocate(allocator, sizeof(vulkan_context_t));
    context->allocator = NULL;
    darray_vulkan_mesh_create  (30, &context->meshes);
    darray_vulkan_shader_create(30, &context->shaders);
    darray_vulkan_image_create (30, &context->images);
    darray_vulkan_material_create(30, &context->materials);
    darray_mat4_create(2048, &context->local_instance_buffer);

    create_vulkan_instance(application_name);
    create_vulkan_debug_callback();

    platform_create_vulkan_surface(plat_state, context->instance, context->allocator, &context->surface);

    get_physical_device();
    get_depth_format();

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physical_device.handle, context->surface, &surface_capabilities);
    context->screen_width = surface_capabilities.currentExtent.width;
    context->screen_height = surface_capabilities.currentExtent.height;

    create_logical_device();

    vulkan_command_buffer_create(context, context->transfer_queue.family_index, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &context->copy_command_buffer);
    vulkan_command_buffer_create(context, context->graphics_queue.family_index, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &context->copy_to_graphics_command_buffer);
    vulkan_swapchain_create(context, &context->swapchain);
    create_command_buffers();

    for (u32 i = 0; i < SWAPCHAIN_MAX_IMAGE_COUNT; i++) {
        vulkan_semaphor_create(context, &context->present_complete_semaphores[i]);
        vulkan_semaphor_create(context, &context->render_complete_semaphores[i]);
        vulkan_fence_create(context, &context->in_flight_fences[i]);
    }

    create_renderpasses();

    // Create descriptor pool
    create_descriptor_pools();

    // CREATE BUFFERS
    vulkan_buffer_create(context, 
            VULKAN_CONTEXT_VERTEX_BUFFER_SIZE,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
            &context->graphics_queue, 
            &context->vertex_buffer);

    vulkan_buffer_create(context, 
            VULKAN_CONTEXT_INDEX_BUFFER_SIZE,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
            &context->graphics_queue, 
            &context->index_buffer);

    // Uniform buffers
    mat4 identity = mat4_translation((vec3) { 0, 0, 0 });
    vulkan_buffer_create(context, 
            sizeof(mat4), 
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
            &context->graphics_queue, 
            &context->uniform_buffer_skybox);
    vulkan_buffer_create(context, 
            sizeof(mat4), 
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
            &context->graphics_queue, 
            &context->uniform_buffer_3d);
    vulkan_buffer_create(context, 
            sizeof(mat4), 
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
            &context->graphics_queue, 
            &context->uniform_buffer_ui);
    vulkan_buffer_update(context, &context->uniform_buffer_skybox, &identity, sizeof(identity), 0);
    vulkan_buffer_update(context, &context->uniform_buffer_3d, &identity, sizeof(identity), 0);
    vulkan_buffer_update(context, &context->uniform_buffer_ui, &identity, sizeof(identity), 0);

    for (u32 i = 0; i < SWAPCHAIN_MAX_IMAGE_COUNT; i++) {
        vulkan_buffer_create(context, 
                VULKAN_INDIRECT_BUFFER_SIZE, 
                VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
                &context->graphics_queue, 
                &context->indirect_draw_infos[i].buffer);
        darray_VkDrawIndexedIndirectCommand_create(32, &context->indirect_draw_infos[i].commands);
        darray_indirect_draw_info_create(32, &context->indirect_draw_infos[i].draws);
    }

    vulkan_buffer_create(context, 
            sizeof(mat4) * VULKAN_CONTEXT_INSTANCE_BUFFER_COUNT, 
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
            &context->graphics_queue, 
            &context->instance_buffer);

    // Load default data types
    vulkan_image_create_from_file(context, 
            "assets/textures/pain.png", 
            TEXTURE_FILTER_LINEAR, 
            VK_IMAGE_USAGE_SAMPLED_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            &context->default_texture);

    // Resource allocators
    block_allocator_create(1024, sizeof(vulkan_buffer_t), &context->shader_buffer_allocator);

    resource_t default_shader_res = resource_loader_get_resource("assets/shaders/default", true);
    shader_t* shader = resource_get_shader(&default_shader_res);
    context->default_shader = context->shaders.data[shader->internal_index];

    resource_t default_mat_res = resource_loader_get_resource("assets/resources/materials/default", true);
    context->default_material = context->materials.data[resource_get_material(&default_mat_res)->internal_index];

    STRACE("Created vulkan renderer");
    return true;
}

b8 vulkan_renderer_shutdown() {
    vkDeviceWaitIdle(context->logical_device);

    SINFO("Shutting down vulkan renderer");

    block_allocator_zero_unused_blocks(&context->shader_buffer_allocator);
    for (u32 i = 0; i < context->shader_buffer_allocator.block_count; i++) {
        vulkan_buffer_t* buffer = (vulkan_buffer_t*)context->shader_buffer_allocator.buffer + i;
        if (buffer->handle) {
            vulkan_buffer_destroy(context, buffer);
        }
    }

    for (u32 i = 0; i < context->shaders.count; i++) {
        vulkan_shader_destroy(context, &context->shaders.data[i]);
    }
    for (u32 i = 0; i < context->images.count; i++) {
        vulkan_image_destroy(context, &context->images.data[i]);
    }
    for (u32 i = 0; i < context->materials.count; i++) {
        vulkan_material_destroy(context, &context->materials.data[i]);
    }

    darray_vulkan_mesh_destroy    (&context->meshes);
    darray_vulkan_shader_destroy  (&context->shaders);
    darray_vulkan_material_destroy(&context->materials);
    darray_vulkan_image_destroy   (&context->images);
    darray_mat4_destroy           (&context->local_instance_buffer);
    sfree(context->shader_text_buffer, context->text_buffer_size, MEMORY_TAG_ARRAY);

    vulkan_image_destroy(context, &context->default_texture);

    vulkan_buffer_destroy(context, &context->uniform_buffer_skybox);
    vulkan_buffer_destroy(context, &context->uniform_buffer_3d);
    vulkan_buffer_destroy(context, &context->uniform_buffer_ui);
    vulkan_buffer_destroy(context, &context->vertex_buffer);
    vulkan_buffer_destroy(context, &context->index_buffer);
    vulkan_buffer_destroy(context, &context->instance_buffer);

    for (u32 i = 0; i < BUILTIN_RENDERPASS_ENUM_MAX; i++) {
        vulkan_renderpass_destroy(context, &context->renderpasses[i]);
    }

    for (u32 i = 0; i < SWAPCHAIN_MAX_IMAGE_COUNT; i++) {
        vulkan_semaphor_destroy(context, context->present_complete_semaphores[i]);
        vulkan_semaphor_destroy(context, context->render_complete_semaphores[i]);
        vulkan_fence_destroy(context, context->in_flight_fences[i]);
        vulkan_buffer_destroy(context, &context->indirect_draw_infos[i].buffer);
        darray_VkDrawIndexedIndirectCommand_destroy(&context->indirect_draw_infos[i].commands);
        darray_indirect_draw_info_destroy(&context->indirect_draw_infos[i].draws);
    }

    for (u32 i = 0; i < context->swapchain.image_count; i++) {
        vulkan_command_buffer_destroy(context, &context->command_buffers[i]);
    }
    vulkan_command_buffer_destroy(context, &context->copy_command_buffer);
    vulkan_command_buffer_destroy(context, &context->copy_to_graphics_command_buffer);

    vulkan_swapchain_destroy(context, &context->swapchain);

    vkDestroyDescriptorPool(context->logical_device, context->descriptor_pool, context->allocator);

    vkDestroyDevice(context->logical_device, context->allocator);

    vkDestroySurfaceKHR(context->instance, context->surface, context->allocator);

    // Resource Allocators
    block_allocator_destroy(&context->shader_buffer_allocator);


#ifdef SPARK_DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkDestroyDebugUtilsMessengerEXT");
    SASSERT(vkDestroyDebugUtilsMessenger, "vkDestroyDebugUtilsMessenger not found. Cannot destroy.");
    vkDestroyDebugUtilsMessenger(context->instance, context->debug_messenger, context->allocator);
#endif

    vkDestroyInstance(context->instance, context->allocator);
    STRACE("Finished shutting down vulkan renderer");
    return true;
}
b8 vulkan_renderer_begin_frame() {
    vkWaitForFences(context->logical_device, 1, &context->in_flight_fences[context->current_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(context->logical_device, 1, &context->in_flight_fences[context->current_frame]);

    vkQueueWaitIdle(context->graphics_queue.handle);
    vkAcquireNextImageKHR(context->logical_device, context->swapchain.handle, UINT64_MAX, context->present_complete_semaphores[context->current_frame], NULL, &context->image_index);

    vulkan_command_buffer_t* command_buffer = &context->command_buffers[context->image_index];
    vulkan_command_buffer_record(command_buffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

    return true;
}

void vulkan_renderer_resize(u16 width, u16 height) {
    vkDeviceWaitIdle(context->logical_device);
    context->screen_width = width;
    context->screen_height = height;
    vulkan_swapchain_recreate(context, &context->swapchain);

    for (u32 i = 0; i < BUILTIN_RENDERPASS_ENUM_MAX; i++) {
        vulkan_renderpass_update_framebuffers(context, &context->renderpasses[i]);
    }
}

void render_geometry_in_pass(vulkan_renderpass_t* renderpass, vulkan_command_buffer_t* command_buffer, VkRect2D render_area, u32 draw_index) {
    vulkan_renderpass_begin(renderpass, command_buffer, render_area, context->image_index);
    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = context->screen_width,
        .height = context->screen_height,
        .minDepth = 0,
        .maxDepth = 1,
    };
    vkCmdSetViewport(command_buffer->handle, 0, 1, &viewport);

    VkRect2D scissor = {
        .extent = {
            .width = context->screen_width,
            .height = context->screen_height,
        },
    };
    vkCmdSetScissor(command_buffer->handle, 0, 1, &scissor);

    u32 old_material = INVALID_ID;
    vulkan_indirect_render_info_t* info = &context->indirect_draw_infos[context->current_frame];
    for (u32 i = 0; i < info->draws.count; i++) {
        indirect_draw_info_t* draw = &info->draws.data[i];

        if (old_material != draw->material_index) {
            old_material = draw->material_index;
            vulkan_material_bind(context, command_buffer, material_loader_get_material(draw->material_index));
        }

        vkCmdDrawIndexedIndirect(command_buffer->handle, info->buffer.handle, sizeof(VkDrawIndexedIndirectCommand) * i, 1, sizeof(VkDrawIndexedIndirectCommand));
    }
    vulkan_renderpass_end(command_buffer);
}

void create_indirect_draw_commands(vulkan_renderpass_t* renderpass, vulkan_command_buffer_t* command_buffer, VkRect2D render_area, const renderpass_geometry_t* renderpass_geo, u32 renderpass_index, vulkan_indirect_render_info_t* indirect_info, u32* instance_offset, u32* draw_offset) {
    if (renderpass_geo->geometry_count <= 0) {
        return;
    }
    vulkan_renderpass_begin(renderpass, command_buffer, render_area, context->image_index);
    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = context->screen_width,
        .height = context->screen_height,
        .minDepth = 0,
        .maxDepth = 1,
    };
    vkCmdSetViewport(command_buffer->handle, 0, 1, &viewport);

    VkRect2D scissor = {
        .extent = {
            .width = context->screen_width,
            .height = context->screen_height,
        },
    };
    vkCmdSetScissor(command_buffer->handle, 0, 1, &scissor);

    u32 instance_count = 0;
    u32 old_material = INVALID_ID;
    vulkan_indirect_render_info_t* info = &context->indirect_draw_infos[context->current_frame];
    for (u32 i = 0, command_index = 0; i < renderpass_geo->geometry_count; i++) {
        instance_count++;
        geometry_render_data_t* geometry = &renderpass_geo->geometry[i];
        vulkan_mesh_t* mesh = &context->meshes.data[geometry->mesh.internal_index];
        u32 material = geometry->material->internal_index;

        darray_mat4_push(&context->local_instance_buffer, geometry->model);

        b8 is_instance = false;
        if (i < renderpass_geo->geometry_count - 1) {
            geometry_render_data_t* next_geometry = &renderpass_geo->geometry[i + 1];
            u32 next_material = geometry->material->internal_index;

            is_instance = geometry->mesh.internal_index == next_geometry->mesh.internal_index && next_material == material;
        }

        if (!is_instance) {
            indirect_draw_info_t draw_info = (indirect_draw_info_t) {
                .material_index = material,
                    .command_index = command_index++,
                    .shader_type = renderpass_index,
            };

            // SDEBUG("Drawing mesh: First index: %d, Index Count: %d, Instance Count: %d, Instance Offset: %d, Vertex Offset: %d", mesh->index_start, mesh->index_count, instance_count, *instance_offset, mesh->vertex_start);
            darray_VkDrawIndexedIndirectCommand_push(&indirect_info->commands, (VkDrawIndexedIndirectCommand) {
                    .firstIndex = mesh->index_start,
                    .indexCount = mesh->index_count,
                    .vertexOffset = mesh->vertex_start,
                    .firstInstance = *instance_offset,
                    .instanceCount = instance_count,
                    });
            // SDEBUG("Drawing Mesh. Index Start: %d, Index Count: %d, Vertex Start: %d, Vertex Count: %d, First Instance: %d, Instance Count: %d", mesh->index_start, mesh->index_count, mesh->vertex_start, mesh->vertex_count, *instance_offset, instance_count);

            darray_indirect_draw_info_push(&indirect_info->draws, draw_info);
            *instance_offset += instance_count;
            instance_count = 0;
            if (old_material != material) {
                old_material = material;
                vulkan_material_bind(context, command_buffer, material_loader_get_material(material));
            }

            vkCmdDrawIndexedIndirect(command_buffer->handle, info->buffer.handle, sizeof(VkDrawIndexedIndirectCommand) * *draw_offset, 1, sizeof(VkDrawIndexedIndirectCommand));
            *draw_offset += 1;
        } 
    }
    vulkan_renderpass_end(command_buffer);
}

b8 vulkan_renderer_draw_frame(render_packet_t* packet) {
    vulkan_command_buffer_t* command_buffer = &context->command_buffers[context->image_index];
    VkRect2D render_area = {
        .offset = {
            .x = 0,
            .y = 0,
        },
        .extent = {
            .width = context->screen_width,
            .height = context->screen_height,
        },
    };

    // Bind vertex and index buffers
    u64 vertex_buffer_offset = 0;
    vkCmdBindVertexBuffers(command_buffer->handle, 0, 1, &context->vertex_buffer.handle, &vertex_buffer_offset);
    vkCmdBindIndexBuffer(command_buffer->handle, context->index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

    // Create indirect buffer data
    vulkan_indirect_render_info_t* indirect_info = &context->indirect_draw_infos[context->current_frame];
    indirect_info->draws.count = 0;
    indirect_info->commands.count = 0;
    context->local_instance_buffer.count = 0;

    // Update uniforms
    mat4 translation = mat4_translation(packet->view_pos);
    mat4 rotation = quat_to_mat4(packet->view_rotation);
    mat4 local = mat4_inverse(mat4_mul(rotation, translation));

    mat4 view_project_matrix = mat4_mul(local, packet->projection_matrix);
    mat4 skybox_view_project_matrix = mat4_mul(mat4_inverse(rotation), packet->projection_matrix);


    const mat4 ui_project_matrix = mat4_mul(mat4_scale((vec3) {.x = 2, .y = 2, .z = 1}), mat4_translation((vec3) { .x = -1, .y = -1}));
    vulkan_buffer_update(context, &context->uniform_buffer_skybox, &skybox_view_project_matrix, sizeof(mat4), 0);
    vulkan_buffer_update(context, &context->uniform_buffer_3d, &view_project_matrix, sizeof(mat4), 0);
    vulkan_buffer_update(context, &context->uniform_buffer_ui, &ui_project_matrix, sizeof(mat4), 0);

    // Mesh instances
    darray_VkDrawIndexedIndirectCommand_clear(&indirect_info->commands);
    darray_mat4_clear(&context->local_instance_buffer);
    darray_indirect_draw_info_clear(&indirect_info->draws);

    u32 instance_offset = 0;
    u32 command_offset = 0;
    for (u32 renderpass_index = 0; renderpass_index < BUILTIN_RENDERPASS_ENUM_MAX; renderpass_index++) {
        const renderpass_geometry_t* renderpass = &packet->renderpass_geometry[renderpass_index];
        create_indirect_draw_commands(&context->renderpasses[renderpass_index], command_buffer, render_area, renderpass, renderpass_index, indirect_info, &instance_offset, &command_offset);

        // Draw all of the geometry
        // render_geometry_in_pass(&context->renderpasses[renderpass_index], command_buffer, render_area, command_offset);
        // command_offset += indirect_info->commands.count;
    }

    vulkan_buffer_update(context, &indirect_info->buffer, indirect_info->commands.data, indirect_info->commands.count * sizeof(VkDrawIndexedIndirectCommand), 0);
    vulkan_buffer_update(context, &context->instance_buffer, context->local_instance_buffer.data, context->local_instance_buffer.count * sizeof(mat4), 0);
    return true;
}

b8 vulkan_renderer_end_frame() {
    vulkan_command_buffer_t* command_buffer = &context->command_buffers[context->image_index];
    vulkan_command_buffer_end_recording(command_buffer);

    // Submit
    VkPipelineStageFlags wait_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    vulkan_queue_submit_async(&context->graphics_queue, command_buffer,
            wait_flags, 
            1, context->present_complete_semaphores[context->current_frame],
            1, context->render_complete_semaphores[context->current_frame],
            context->in_flight_fences[context->current_frame]);

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &context->render_complete_semaphores[context->current_frame],
        .swapchainCount = 1,
        .pSwapchains = &context->swapchain.handle,
        .pImageIndices = &context->image_index,
    };
    context->current_frame = (context->current_frame + 1) % context->swapchain.image_count;

    VK_CHECK(vkQueuePresentKHR(context->graphics_queue.handle, &present_info));

    return true;
}

mesh_t vulkan_create_mesh(const void* vertices, u32 vertex_count, u32 vertex_stride, const void* indices, u32 index_count, u32 index_stride) {
    SASSERT(vertex_stride > 0, "Cannot add vertices with stride of 0.");
    SASSERT(index_stride > 0,  "Cannot add indices with stride of 0.");
    // SDEBUG("Creating mesh: Vertex count: %d, Stride: %d, Offset: %d, Index count: %d, Stride: %d, Offset: %d", vertex_count, vertex_stride, context->vertex_buffer_offset, index_count, index_stride, context->index_buffer_offset);
    SASSERT(index_stride == 4, "Short index not implemented.");

    // Pad the vertex buffer so that shaders can have unique vertex layouts
    u32 index_padding = index_stride - (context->index_buffer_offset % index_stride);
    if (index_padding != index_stride) {
        context->index_buffer_offset += index_padding;
    }
    u32 vertex_padding = vertex_stride - (context->vertex_buffer_offset % vertex_stride);
    if (vertex_padding != vertex_stride) {
        context->vertex_buffer_offset += vertex_padding;
    }

    SASSERT(context->vertex_buffer_offset % vertex_stride == 0, "Invalid vbo");
    SASSERT(context->index_buffer_offset % index_stride == 0, "Invalid vbo");

    vulkan_mesh_t internal_mesh = {
        .index_count = index_count,
        .index_start = context->index_buffer_offset / index_stride,
        .vertex_count = vertex_count,
        .vertex_stride = vertex_stride,
        .vertex_start = context->vertex_buffer_offset / vertex_stride,
    };
    // SDEBUG("\tIndext Start: %d, Vertex Start: %d", internal_mesh.index_start, internal_mesh.vertex_start);

    // Set buffers
    vulkan_buffer_update(context, &context->vertex_buffer, vertices, vertex_count * vertex_stride, context->vertex_buffer_offset);
    context->vertex_buffer_offset += vertex_count * vertex_stride;

    if (index_count > 0) {
        vulkan_buffer_update(context, &context->index_buffer, indices, index_count * index_stride, context->index_buffer_offset);
        context->index_buffer_offset += index_count * index_stride;
    }

    // Get new index
    u32 internal_index = context->meshes.count;
    darray_vulkan_mesh_push(&context->meshes, internal_mesh);

    mesh_t mesh = {
        .internal_index = internal_index,
    };

    return mesh;
}

shader_t vulkan_create_shader(shader_config_t* config) {
    // Shader
    vulkan_shader_t shader = {};

    if (config->vertex_spv) {
        vulkan_shader_module_create(context, (u32*)config->vertex_spv, config->vertex_spv_size, &shader.vert);
    }
    if (config->fragment_spv) {
        vulkan_shader_module_create(context, (u32*)config->fragment_spv, config->fragment_spv_size, &shader.frag);
    }

    SASSERT(config->type >= 0 && config->type < BUILTIN_RENDERPASS_ENUM_MAX, "Cannot create shader for invalid renderpass: %d", config->type);
    vulkan_renderpass_t* renderpass = &context->renderpasses[config->type];
    vulkan_shader_create(context, renderpass, 0, config->attribute_count, config->attributes, config->resource_count, config->layout, true, &shader);

    // Bind shader globals
    shader_resource_t skybox_uniform_resource[] = { 
        {
            .layout = {
                .binding = 0,
                .set = 0,
                .type = SHADER_RESOURCE_UNIFORM_BUFFER,
            },
            .data = &context->uniform_buffer_skybox,
        },
    };
    shader_resource_t world_uniform_resource[] = { 
        {
            .layout = {
                .binding = 0,
                .set = 0,
                .type = SHADER_RESOURCE_UNIFORM_BUFFER,
            },
            .data = &context->uniform_buffer_3d,
        },
        {
            .layout = {
                .binding = 1,
                .set = 0,
                .type = SHADER_RESOURCE_SOTRAGE_BUFFER,
            },
            .data = &context->instance_buffer,
        },
    };
    shader_resource_t ui_uniform_resource = {
        .layout = {
            .binding = 0,
            .set = 0,
            .type = SHADER_RESOURCE_UNIFORM_BUFFER,
        },
        .data = &context->uniform_buffer_ui,
    };

    switch (config->type) {
        case BUILTIN_RENDERPASS_SKYBOX:
            vulkan_shader_bind_resources(context, &shader, 0, 1, skybox_uniform_resource);
            break;
        case BUILTIN_RENDERPASS_WORLD:
            vulkan_shader_bind_resources(context, &shader, 0, 2, world_uniform_resource);
            break;
        case BUILTIN_RENDERPASS_UI:
            vulkan_shader_bind_resources(context, &shader, 0, 1, &ui_uniform_resource);
            break;
        case BUILTIN_RENDERPASS_ENUM_MAX:
            break;
    }

    shader_t shader_base = {
        .resource_count  = config->resource_count,
        .attribute_count = config->attribute_count,
        .internal_index  = context->shaders.count,
        .renderpass            = config->type,
    };
    scopy_memory(shader_base.attributes, config->attributes, sizeof(config->attributes));
    scopy_memory(shader_base.layout, config->layout, sizeof(config->layout));

    darray_vulkan_shader_push(&context->shaders, shader);

    return shader_base;
}

texture_t vulkan_create_image_from_file(const char* path, texture_filter_t filter) {
    vulkan_image_t texture;
    vulkan_image_create_from_file(context, path, filter, VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texture);

    texture_t out_texture = {
        .internal_index = context->images.count,
    };

    darray_vulkan_image_push(&context->images, texture);

    return out_texture;
}
texture_t vulkan_create_image_from_data(const char* data, u32 width, u32 height, u32 channels, texture_filter_t filter) {
    vulkan_image_t texture;
    vulkan_image_create(context, filter, VK_FORMAT_R8G8B8A8_SRGB, width, height, VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texture);
    vulkan_image_upload_pixels(context, VK_FORMAT_R8G8B8A8_SRGB, width, height, (const void*)data, &texture);

    texture_t out_texture = {
        .internal_index = context->images.count,
    };

    darray_vulkan_image_push(&context->images, texture);

    return out_texture;
}

material_t vulkan_create_material(material_config_t* config) {
    vulkan_material_t material = {
        .first_set = 255
    };

    // Create descriptor sets
    vulkan_shader_t internal_shader = context->default_shader;
    u32 shader_index = 0;
    if (config->shader_path[0] != 0) {
        resource_t shader_res = resource_loader_get_resource(config->shader_path, true);
        shader_t* shader = resource_get_shader(&shader_res);
        internal_shader = context->shaders.data[shader->internal_index];
        shader_index = shader->internal_index;
    }

    // Allocate resources
    for (u32 set = 0; set < VULKAN_MATERIAL_MAX_SETS; set++) {
        u32 sampler_count = 0;
        u32 uniform_count = 0;
        u32 storage_count = 0;

        for (u32 i = 0; i < config->resource_count; i++) {
            const material_resource_t* resource = &config->resources[i];
            // Skip sets that aren't the current set
            SASSERT(resource->set >= 0 && resource->set < VULKAN_MATERIAL_MAX_SETS, "Material resource set '%d' is not within bounds. Must be >= 0 and < %d.", resource->set, VULKAN_MATERIAL_MAX_SETS);
            if (resource->set != set) {
                continue;
            }

            switch (resource->type) {
                case SHADER_RESOURCE_SOTRAGE_BUFFER:
                    storage_count++;
                    break;
                case SHADER_RESOURCE_UNIFORM_BUFFER:
                    uniform_count++;
                    break;
                case SHADER_RESOURCE_SAMPLER:
                    sampler_count++;
                    break;
                case SHADER_RESOURCE_UNDEFINED:
                    SERROR("Cannot create undefined shader resource.");
                    break;
            }
        }

        u32 unique_set_count = 0;
        if (sampler_count > 0) {
            unique_set_count++;
        }
        if (uniform_count > 0) {
            unique_set_count++;
        }
        if (storage_count > 0) {
            unique_set_count++;
        }

        SASSERT(unique_set_count <= 1, "Vulkan Material cannot create two sets with different types of data.");
        if (unique_set_count <= 0) {
            continue;
        }

        if (set < material.first_set) {
            material.first_set = set;
        }
        material.set_count++;

        if (sampler_count > 0) {
            VkDescriptorSetAllocateInfo allocate_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = context->descriptor_pool,
                .descriptorSetCount = sampler_count,
                .pSetLayouts = &internal_shader.descriptor_layouts[set],
            };

            vkAllocateDescriptorSets(context->logical_device, &allocate_info, &material.sets[set]);
        }
        if (uniform_count > 0) {
            VkDescriptorSetAllocateInfo allocate_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = context->descriptor_pool,
                .descriptorSetCount = uniform_count,
                .pSetLayouts = &internal_shader.descriptor_layouts[set],
            };

            vkAllocateDescriptorSets(context->logical_device, &allocate_info, &material.sets[set]);
        }
        if (storage_count > 0) {
            VkDescriptorSetAllocateInfo allocate_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = context->descriptor_pool,
                .descriptorSetCount = storage_count,
                .pSetLayouts = &internal_shader.descriptor_layouts[set],
            };

            vkAllocateDescriptorSets(context->logical_device, &allocate_info, &material.sets[set]);
        }
    }

    // Update descriptor sets
    VkDescriptorBufferInfo buffer_info[VULKAN_MATERIAL_MAX_SETS];
    VkDescriptorImageInfo image_infos[VULKAN_MATERIAL_MAX_SETS];
    VkWriteDescriptorSet descriptor_writes[VULKAN_MATERIAL_MAX_SETS] = {};

    vulkan_image_t* image = NULL;
    for (u32 i = 0; i < config->resource_count; i++) {
        u32 set = config->resources[i].set;

        switch (config->resources[i].type) {
            case SHADER_RESOURCE_UNDEFINED:
                SERROR("Cannot use undefined shader resource.");
                break;
            case SHADER_RESOURCE_SOTRAGE_BUFFER:
                if (config->resources[i].value == NULL) {
                    vulkan_buffer_t* buffer = block_allocator_allocate(&context->shader_buffer_allocator);
                    vulkan_buffer_create(context, 
                            4096,
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
                            &context->graphics_queue, 
                            buffer);
                    config->resources[i].value = buffer;
                    material.storage_buffer = buffer;
                }
                vulkan_buffer_create_descriptor_write(config->resources[i].value, config->resources[i].binding, material.sets[set], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &buffer_info[i], &descriptor_writes[i]);
                break;
            case SHADER_RESOURCE_UNIFORM_BUFFER:
                vulkan_buffer_create_descriptor_write(config->resources[i].value, config->resources[i].binding, material.sets[set], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &buffer_info[i], &descriptor_writes[i]);
                break;
            case SHADER_RESOURCE_SAMPLER:
                image = &context->images.data[((texture_t*)config->resources[i].value)->internal_index];
                image_infos[i].sampler     = image->sampler;
                image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_infos[i].imageView   = image->view;

                descriptor_writes[i] = 
                    (VkWriteDescriptorSet) {
                        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet          = material.sets[set],
                        .dstBinding      = config->resources[i].binding,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .pImageInfo      = &image_infos[i],
                        .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    };
                break;
        }
    }

    vkUpdateDescriptorSets(context->logical_device, config->resource_count, descriptor_writes, 0, NULL);

    u32 internal_index = context->materials.count;
    darray_vulkan_material_push(&context->materials, material);
    material_t out_material = {
        .internal_index = internal_index,
        .shader_index = shader_index,
    };

#ifdef SPARK_DEBUG
    string_copy(out_material.name, config->name);
#endif

    return out_material;
}

vec2 vulkan_get_screen_size() {
    return (vec2) {
        .x = context->screen_width,
            .y = context->screen_height,
    };
}
// ==================================
// Private function implementations
// ==================================
void create_descriptor_pools() {
    // Create pool
    VkDescriptorPoolSize pool_sizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = VULKAN_CONTEXT_MAX_STORAGE_BUFFERS,
        }, {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = VULKAN_CONTEXT_MAX_UNIFORM_BUFFERS,
        }, {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = VULKAN_CONTEXT_MAX_SAMPLERS,
        }
    };

    const u32 pool_count = sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize);
    VkDescriptorPoolCreateInfo descriptor_pool_create = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = pool_count,
        .pPoolSizes    = pool_sizes,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = VULKAN_CONTEXT_MAX_DESCRIPTOR_SETS,
    };

    VK_CHECK(vkCreateDescriptorPool(context->logical_device, &descriptor_pool_create, context->allocator, &context->descriptor_pool));
}

void create_vulkan_instance(const char* application_name) {
    // ==================================
    // Get API version
    // ==================================
    u32 api_version = 0;
    VK_CHECK(vkEnumerateInstanceVersion(&api_version));

    SINFO("Drivers supports vulkan version %d.%d.%d.%d - Using vulkan 0.1.2.x", 
            VK_API_VERSION_VARIANT(api_version),
            VK_API_VERSION_MAJOR(api_version),
            VK_API_VERSION_MINOR(api_version),
            VK_API_VERSION_PATCH(api_version));

    SASSERT(VK_API_VERSION_MAJOR(api_version) >= 1, "Cannot use vulkan. Invalid version: %d.%d.%d.%d (must be at least 0.1.2.0)", 
            VK_API_VERSION_VARIANT(api_version),
            VK_API_VERSION_MAJOR(api_version),
            VK_API_VERSION_MINOR(api_version),
            VK_API_VERSION_PATCH(api_version));

    SASSERT(VK_API_VERSION_MINOR(api_version) >= 2, "Cannot use vulkan. Invalid version: %d.%d.%d.%d (must be at least 0.1.2.0)", 
            VK_API_VERSION_VARIANT(api_version),
            VK_API_VERSION_MAJOR(api_version),
            VK_API_VERSION_MINOR(api_version),
            VK_API_VERSION_PATCH(api_version));

    // ==================================
    // Create application info
    // ==================================
    VkApplicationInfo application_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = application_name,
        .pEngineName = "Spark",
        .applicationVersion = 1,
        .apiVersion = VK_API_VERSION_1_2,
    };

    // ==================================
    // Get Layers
    // ==================================
    const u32 MAX_LAYER_COUNT = 32;
    const char* vulkan_layer_names[MAX_LAYER_COUNT];
    u32 layer_count = 0;

#ifdef SPARK_DEBUG
    vulkan_layer_names[layer_count++] = "VK_LAYER_KHRONOS_validation";
#endif

    SASSERT(layer_count < MAX_LAYER_COUNT, "Vulkan trying to add too many extensions");

    for (u32 i = 0; i < layer_count; i++) {
        STRACE("Using vulkan layer '%s", vulkan_layer_names[i]);
    }

    // ==================================
    // Get Extensions
    // ==================================
    const u32 MAX_EXTENSION_COUNT = 32;
    const char* extension_names[MAX_EXTENSION_COUNT];
    extension_names[0] = "VK_KHR_surface";
    /*extension_names[2] = VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME;*/
    u32 extension_count = 1;

#ifdef SPARK_DEBUG
    extension_names[extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#endif

    u32 platform_extension_count = -1;
    platform_get_required_extension_names(extension_names, extension_count, &platform_extension_count);

    SASSERT(platform_extension_count != -1, "Failed to get platform extension count. Please remember to set it in the platform_get_required_extension_names function");
    SASSERT(extension_count < MAX_EXTENSION_COUNT, "Vulkan trying to add too many extensions");

    extension_count += platform_extension_count;


    for (u32 i = 0; i < extension_count; i++) {
        STRACE("Using vulkan extension '%s'", extension_names[i]);
    }

    // ==================================
    // Create instance
    // ==================================
    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .flags = 0,
        .pApplicationInfo = &application_info,
        .ppEnabledExtensionNames = extension_names,
        .enabledExtensionCount = extension_count,
        .ppEnabledLayerNames = vulkan_layer_names,
        .enabledLayerCount = layer_count,
    };

    VK_CHECK(vkCreateInstance(&create_info, context->allocator, &context->instance));
}

void create_vulkan_debug_callback() {
#ifdef SPARK_DEBUG
    VkDebugUtilsMessengerCreateInfoEXT messenger_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        .pfnUserCallback = vulkan_debug_callback,
        .pUserData = NULL,
    };

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkCreateDebugUtilsMessengerEXT");
    SASSERT(vkCreateDebugUtilsMessenger, "Failed to get vkCreateDebugUtilsMessenger");

    VK_CHECK(vkCreateDebugUtilsMessenger(context->instance, &messenger_create_info, context->allocator, &context->debug_messenger));
#endif
}

VkBool32 vulkan_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data) {

    switch (severity) {
        default:
            return VK_FALSE;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            STRACE(callback_data->pMessage);
            break;
            /*case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:*/
            /*    SINFO(callback_data->pMessage);*/
            /*    break;*/
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            SWARN(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            SCRITICAL(callback_data->pMessage);
            break;
    }

    return VK_FALSE;
}

void get_physical_device() {
    // Get list of devices
    const u32 MAX_DEVICE_COUNT = 32; // This is a lot of GPUs 
    u32 device_count = MAX_DEVICE_COUNT; // NOTE: this gets overwritten to the real device count by vkEnumeratePhysicalDevices
    VkPhysicalDevice devices[MAX_DEVICE_COUNT];

    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &device_count, devices));

    SASSERT(device_count < MAX_DEVICE_COUNT, "This device has more VkPhysicalDevices than capable of using. Device has %d, MAX_DEVICE_COUNT: %d", device_count, MAX_DEVICE_COUNT);
    SASSERT(device_count > 0, "Vulkan failed to find any physical devices");

    // Score each device
    u32 best_score = 0;
    u32 best_index = 0;
    for (u32 i = 0; i < device_count; i++) {
        u32 score = get_physical_device_score(devices[i]);
        if (score > best_score) {
            best_score = score;
            best_index = i;
        }
    }

    STRACE("Using physical device %d", best_index);

    // Use the best physical device
    context->physical_device.handle = devices[best_index];

    // Grab physical device info
    vkGetPhysicalDeviceMemoryProperties(context->physical_device.handle, &context->physical_device.memory_properties);
}

u32 get_physical_device_score(VkPhysicalDevice device) {
    // Get properties
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    STRACE("Scoring physical device: '%s'", properties.deviceName);
    STRACE("\tAPI Version: %d.%d.%d.%d", 
            VK_API_VERSION_VARIANT(properties.apiVersion), 
            VK_API_VERSION_MAJOR(properties.apiVersion), 
            VK_API_VERSION_MINOR(properties.apiVersion), 
            VK_API_VERSION_PATCH(properties.apiVersion));
    STRACE("\tDriver Version: 0x%x", properties.driverVersion);

    // Start scoring
    u32 score = 0;
    // Score the device based on discrete or integrated
    switch (properties.deviceType) {
        default:
            break;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            score += 0;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score += 1;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score += 2;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score += 3;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score += 4;
            break;
    }

    // Get device groups
    const u32 MAX_QUEUE_FAMILIY_COUNT = 32;
    u32 queue_family_count = MAX_QUEUE_FAMILIY_COUNT;

    VkQueueFamilyProperties queue_family_properties[MAX_QUEUE_FAMILIY_COUNT];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_properties);

    SASSERT(queue_family_count < MAX_QUEUE_FAMILIY_COUNT, "Cannot get device queue families. Got %d groups, max: %d", queue_family_count, MAX_QUEUE_FAMILIY_COUNT);
    STRACE("\tQueue family count: %d", queue_family_count);

    b8 device_supports_present = false;
    b8 device_supports_graphics = false;
    b8 device_supports_transfer = false;
    b8 device_supports_compute = false;
    for (u32 i = 0; i < queue_family_count; i++) {
        const VkQueueFamilyProperties queue_properties = queue_family_properties[i];
        if (queue_properties.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            device_supports_compute = true;
        }
        if (queue_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            device_supports_graphics = true;
        }
        if (queue_properties.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            device_supports_transfer = true;
        }

        VkBool32 present_supported = false;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, context->surface, &present_supported));
        if (present_supported) {
            device_supports_present = true;
        } 
    }

    if (!device_supports_present || !device_supports_graphics ||
            !device_supports_compute || !device_supports_transfer) {
        SWARN("\tDevice does not meet requirements.");
        return 0;
    }

    return score;
}


u32 get_most_unique_queue_index(u32 queue_family_count, const VkQueueFamilyProperties* queue_family_properties, VkQueueFlags flags, b8 use_present) {

    u32 best_index = INVALID_ID;

    u32 min_shared_queue_count = UINT32_MAX;
    for (u32 i = 0; i < queue_family_count; i++) {
        const VkQueueFamilyProperties queue_properties = queue_family_properties[i];
        if (!(queue_properties.queueFlags & flags) && flags) {
            continue;
        }

        VkBool32 present_supported = false;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(context->physical_device.handle, i, context->surface, &present_supported));
        if (use_present && !present_supported) {
            continue;
        } 

        // Found queue with required data
        u32 shared_queue_count = 0;
        if (present_supported) {
            shared_queue_count++;
        }

        for (u32 j = 1; j < VK_QUEUE_FLAG_BITS_MAX_ENUM; j <<= 1) {
            if (queue_properties.queueFlags & j) {
                shared_queue_count++;
            }
        }

        if (shared_queue_count < min_shared_queue_count) {
            best_index = i;
            min_shared_queue_count = shared_queue_count;
        }
    }

    SASSERT(best_index != INVALID_ID, "Failed to get VkQueue for flags 0x%x and use_present %d", flags, use_present);
    return best_index;
}

void create_logical_device() {
    STRACE("Creating logical device");
    // Create list of queues to create
    // Get queue families
    const u32 MAX_QUEUE_FAMILY_COUNT = 32;
    u32 queue_family_count = MAX_QUEUE_FAMILY_COUNT;
    VkQueueFamilyProperties queue_family_properties[MAX_QUEUE_FAMILY_COUNT];
    vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device.handle, &queue_family_count, queue_family_properties);

    SASSERT(queue_family_count < MAX_QUEUE_FAMILY_COUNT, "Cannot get device queue families. Got %d groups, max: %d", queue_family_count, MAX_QUEUE_FAMILY_COUNT);
    STRACE("Queue family count: %d", queue_family_count);

    // Get most unique queue for compute, transfer, graphics, and present
    context->compute_queue.family_index = get_most_unique_queue_index(queue_family_count, queue_family_properties, VK_QUEUE_COMPUTE_BIT, false);
    context->graphics_queue.family_index = get_most_unique_queue_index(queue_family_count, queue_family_properties, VK_QUEUE_GRAPHICS_BIT, false);
    context->transfer_queue.family_index = get_most_unique_queue_index(queue_family_count, queue_family_properties, VK_QUEUE_TRANSFER_BIT, false);
    context->present_queue.family_index = get_most_unique_queue_index(queue_family_count, queue_family_properties, 0, false);
    u32 queue_indices[4] = {
        context->compute_queue.family_index,
        context->graphics_queue.family_index,
        context->transfer_queue.family_index,
        context->present_queue.family_index,
    };

    u32 queue_count = 0;
    u32 unique_queue_indices[4] = {
        INVALID_ID,
        INVALID_ID,
        INVALID_ID,
        INVALID_ID,
    };

    for (u32 i = 0; i < 4; i++) {
        b8 is_unique = true;
        for (u32 j = 0; j < 4; j++) {
            if (queue_indices[i] == unique_queue_indices[j]) {
                is_unique = false;
                break;
            }
        }

        if (!is_unique) {
            continue;
        }

        unique_queue_indices[queue_count] = queue_indices[i];
        queue_count++;
    }

    // Create queue create infos
    const float queue_priorities[4] = {
        1.0f,
        1.0f,
        1.0f,
        1.0f,
    };

    VkDeviceQueueCreateInfo queue_create_infos[4];
    for (u32 i = 0; i < queue_count; i++) {
        queue_create_infos[i] = (VkDeviceQueueCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueCount = 1,
                .pQueuePriorities = queue_priorities,
                .queueFamilyIndex = unique_queue_indices[i],
        };
    }

    // Create device
    VkPhysicalDeviceFeatures device_features = {};
    const char* device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = queue_count,
        .pQueueCreateInfos = queue_create_infos,
        .pEnabledFeatures = &device_features,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = &device_extensions,
    };

    VK_CHECK(vkCreateDevice(context->physical_device.handle, &create_info, context->allocator, &context->logical_device));

    // Get the created queues
    vkGetDeviceQueue(context->logical_device, context->graphics_queue.family_index, 0, &context->graphics_queue.handle);
    vkGetDeviceQueue(context->logical_device, context->compute_queue.family_index, 0, &context->compute_queue.handle);
    vkGetDeviceQueue(context->logical_device, context->transfer_queue.family_index, 0, &context->transfer_queue.handle);
    vkGetDeviceQueue(context->logical_device, context->present_queue.family_index, 0, &context->present_queue.handle);
}

void get_depth_format() {
    VkFormat possible_formats[3] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };
    const u32 possible_format_count = sizeof(possible_formats) / sizeof(VkFormat);
    const VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    for (u32 i = 0; i < possible_format_count; i++) {
        VkFormatProperties format_properties;
        vkGetPhysicalDeviceFormatProperties(context->physical_device.handle, possible_formats[i], &format_properties);

        if ((format_properties.optimalTilingFeatures & features) == features) {
            context->physical_device.depth_format = possible_formats[i];
            return;
        }
    }

    SCRITICAL("Failed to get format for depth buffer");
}

void create_command_buffers() {
    for (u32 i = 0; i < context->swapchain.image_count; i++) {
        vulkan_command_buffer_create(context, context->graphics_queue.family_index, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &context->command_buffers[i]);
    }
}

void create_renderpasses() {
    VkClearValue clear_color = {
        .color = {
            .float32 = {
                0.0f,
                0.5f,
                1.0f,
                0.0f,
            }
        }
    };

    vulkan_renderpass_create(context, clear_color, true, true, context->screen_width, context->screen_height, &context->renderpasses[BUILTIN_RENDERPASS_SKYBOX]);
    vulkan_renderpass_create(context, clear_color, true, false,   context->screen_width, context->screen_height, &context->renderpasses[BUILTIN_RENDERPASS_WORLD]); 
    vulkan_renderpass_create(context, clear_color, false, false, context->screen_width, context->screen_height, &context->renderpasses[BUILTIN_RENDERPASS_UI]);
}

void vulkan_renderer_material_update_buffer(material_t* material, void* data, u32 size, u32 offset) {
    vulkan_material_t* vulkan_material = &context->materials.data[material->internal_index];
    vulkan_material_update_buffer(context, data, size, offset, vulkan_material);
}
