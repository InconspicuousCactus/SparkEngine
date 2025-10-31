#include "Spark/renderer/vulkan/vulkan_material.h"
#include "Spark/containers/darray.h"
#include "Spark/renderer/vulkan/vulkan_buffer.h"
#include "Spark/renderer/vulkan/vulkan_command_buffer.h"
#include "Spark/renderer/vulkan/vulkan_context.h"
#include <vulkan/vulkan_core.h>

darray_impl(vulkan_material_t, vulkan_material);

void vulkan_material_create(struct vulkan_context* context, vulkan_material_t* out_material) {

}

void vulkan_material_destroy(struct vulkan_context* context, vulkan_material_t* material) {
    if (material->set_count <= 0) {
        return;
    }
    vkFreeDescriptorSets(context->logical_device, context->descriptor_pool, material->set_count, &material->sets[material->first_set]);
}

void vulkan_material_bind(vulkan_context_t* context, vulkan_command_buffer_t* command_buffer, material_t* material) {
    vulkan_shader_t* shader = &context->shaders.data[material->shader_index];
    vulkan_material_t* internal_material = &context->materials.data[material->internal_index];
    SASSERT(internal_material != NULL, "Cannot bind material (index: %d): No internal material created.", material->internal_index);
    if (internal_material->set_count == 0) {
        vulkan_shader_bind(&context->default_shader, command_buffer);
        VkDescriptorSet* pDescriptorSets = &context->default_material.sets[context->default_material.first_set];
        vkCmdBindDescriptorSets(command_buffer->handle, 
                VK_PIPELINE_BIND_POINT_GRAPHICS, 
                context->default_shader.pipeline_layout, 
                context->default_material.first_set, context->default_material.set_count, pDescriptorSets, 
                0, NULL);
        return;
    }

    vulkan_shader_bind(shader, command_buffer);
    vkCmdBindDescriptorSets(command_buffer->handle, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, 
            shader->pipeline_layout, 
            internal_material->first_set, internal_material->set_count, &internal_material->sets[internal_material->first_set], 
            0, NULL);
    return;
}

void vulkan_material_update_buffer(struct vulkan_context* context, void* data, u32 size, u32 offset, vulkan_material_t* material) {
    vulkan_buffer_update(context, material->storage_buffer, data, size, offset);
}
