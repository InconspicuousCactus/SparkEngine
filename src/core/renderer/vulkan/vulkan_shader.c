#include "Spark/renderer/vulkan/vulkan_shader.h"
#include "Spark/core/smemory.h"
#include "Spark/core/sstring.h"
#include "Spark/math/mat4.h"
#include "Spark/platform/filesystem.h"
#include "Spark/renderer/material.h"
#include "Spark/renderer/shader.h"
#include "Spark/renderer/vulkan/vulkan_context.h"
#include "Spark/renderer/vulkan/vulkan_utils.h"
#include "Spark/types/s3d.h"
#include <vulkan/vulkan_core.h>

// ========================
// Private functions
// ========================
void create_shader_descriptors(vulkan_context_t* context, u32 set, u32 resource_count, shader_resource_layout_t* resources, vulkan_shader_t* out_shader);

void vulkan_shader_module_create_from_file(struct vulkan_context* context, const char* path, VkShaderModule* out_module) {
    SASSERT(filesystem_exists(path), "Cannot create shader from file '%s' because it cannot be found", path);

    file_handle_t handle;
    filesystem_open(path, FILE_MODE_READ, true, &handle);

    u64 size;
    filesystem_size(&handle, &size);

    if (size > context->text_buffer_size) {
        if (context->shader_text_buffer) {
            sfree((void*)context->shader_text_buffer, context->text_buffer_size, MEMORY_TAG_ARRAY);
        }

        context->text_buffer_size = 2 * size;
        context->shader_text_buffer = sallocate(context->text_buffer_size, MEMORY_TAG_ARRAY);
    }

    u64 bytes_read = 0;
    filesystem_read(&handle, size, (void*)context->shader_text_buffer, &bytes_read);
    filesystem_close(&handle);

    vulkan_shader_module_create(context, (u32*)context->shader_text_buffer, bytes_read, out_module);
}

void vulkan_shader_module_create(struct vulkan_context* context, const u32* spirv, u32 spirv_size, VkShaderModule* out_module) {
    VkShaderModuleCreateInfo create_info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = NULL,
        .flags    = 0,
        .pCode    = spirv,
        .codeSize = spirv_size,
    };

    vkCreateShaderModule(context->logical_device, &create_info, context->allocator, out_module);
}

void vulkan_shader_create(struct vulkan_context* context, 
        vulkan_renderpass_t* renderpass, 
        u32 subpass, 
        u32 vertex_attribute_count, 
        vertex_attributes_t* vertex_attributes, 
        u32 resource_count, 
        shader_resource_layout_t* resources, 
        u8 use_depth, 
        vulkan_shader_t* out_shader) {

    out_shader->resource_count = resource_count;
    out_shader->set_count = 0;

    for (u32 i = 0; i < VULKAN_SHADER_MAX_SETS; i++) {
        create_shader_descriptors(context, i, resource_count, resources, out_shader);
    }

    // Get all stages,
    SASSERT(out_shader->vert != VK_NULL_HANDLE, "Vulkan Shader does not have vertex module.");

    VkPipelineShaderStageCreateInfo stages[VULKAN_SHADER_MAX_STAGES];
    u32 stage_count = 1;
    stages[0] = (VkPipelineShaderStageCreateInfo) {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_VERTEX_BIT,
            .module = out_shader->vert,
            .pName  = "main"
    };
    if (out_shader->frag != VK_NULL_HANDLE) {
        stages[stage_count++] = (VkPipelineShaderStageCreateInfo) {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = out_shader->frag,
                .pName  = "main"
        };
    }

    VkPipelineTessellationStateCreateInfo tessellation = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO,
    };

    // Create input assembly info

    VkVertexInputAttributeDescription vertex_attribute_descriptions[SHADER_MAX_VERTEX_ATTRIBUTES];
    u32 vertex_stride = 0;
    for (u32 i = 0, offset = 0; i < vertex_attribute_count; i++) {
        VkFormat format = VK_FORMAT_UNDEFINED;
        u32 stride = 0;
        switch (vertex_attributes[i]) {
        case VERTEX_ATTRIBUTE_NONE:
            SERROR("Shader cannot use vertex attribute 'None'");
            format = VK_FORMAT_R32_SFLOAT;
            stride = 0;
            break;
        case VERTEX_ATTRIBUTE_FLOAT:
            format = VK_FORMAT_R32_SFLOAT;
            stride = sizeof(float);
            break;
        case VERTEX_ATTRIBUTE_VEC2:
            format = VK_FORMAT_R32G32_SFLOAT;
            stride = sizeof(vec2);
            break;
        case VERTEX_ATTRIBUTE_VEC3:
            format = VK_FORMAT_R32G32B32_SFLOAT;
            stride = sizeof(vec3);
            break;
        case VERTEX_ATTRIBUTE_VEC4:
            format = VK_FORMAT_R32G32B32A32_SFLOAT;
            stride = sizeof(vec4);
            break;
        case VERTEX_ATTRIBUTE_INT:
            format = VK_FORMAT_R32_SINT;
            stride = sizeof(int);
            break;
        case VERTEX_ATTRIBUTE_IVEC2:
            format = VK_FORMAT_R32G32_SINT;
            stride = sizeof(vec2i);
            break;
        case VERTEX_ATTRIBUTE_IVEC3:
            format = VK_FORMAT_R32G32B32_SINT;
            stride = sizeof(vec3i);
            break;
        case VERTEX_ATTRIBUTE_IVEC4:
            format = VK_FORMAT_R32G32B32A32_SINT;
            stride = sizeof(vec4i);
            break;
        }
        SASSERT(format != VK_FORMAT_UNDEFINED, "Vulkan shadder failed to select format for attribute 0x%x", vertex_attributes[i]);
        vertex_attribute_descriptions[i] = 
            (VkVertexInputAttributeDescription) {
                .binding = 0,
                .location = i,
                .format = format,
                .offset = offset,
            };

        vertex_stride += stride;
        offset += stride;
    }

#ifdef SPARK_DEBUG
    out_shader->vertex_stride = vertex_stride;
#endif

    VkVertexInputBindingDescription vertex_descriptors[] = {
        (VkVertexInputBindingDescription) 
        {
            .binding = 0,
            .stride = vertex_stride,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    };
    VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType                         = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .flags                         = 0,
        .vertexBindingDescriptionCount = sizeof(vertex_descriptors) / sizeof(VkVertexInputBindingDescription),
        .pVertexBindingDescriptions    = vertex_descriptors,
        .vertexAttributeDescriptionCount = vertex_attribute_count,
        .pVertexAttributeDescriptions = vertex_attribute_descriptions,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    // Viewport
    VkRect2D scissor = {
        .offset = {
            .x = 0,
            .y = 0,
        },
        .extent = {
            .width  = context->screen_width,
            .height = context->screen_height,
        },
    };

    VkViewport viewports = {
        .x        = 0, 
        .y        = 0,
        .width    = context->screen_width,
        .height   = context->screen_height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkPipelineViewportStateCreateInfo viewport_create = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports    = &viewports,
        .scissorCount  = 1,
        .pScissors     = &scissor,
    };

    // Dynamic state
    VkDynamicState dynamic_states[] = { 
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = sizeof(dynamic_states) / sizeof(VkDynamicState),
        .pDynamicStates = dynamic_states,
    };

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasterization = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthBiasEnable         = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_FRONT_BIT,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth               = 1.0f,
    };

    // Sampling
    VkPipelineMultisampleStateCreateInfo multisample = {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable  = VK_FALSE,
        .minSampleShading     = 1.0f,
    };

    // Color blend
    VkPipelineColorBlendAttachmentState color_attachment = {
        .blendEnable    = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,
    };

    VkPipelineColorBlendStateCreateInfo color_blend = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, 
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments    = &color_attachment,
    };

    // Depth
    VkPipelineDepthStencilStateCreateInfo depth_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = true,
        .depthWriteEnable = true,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };


    // Layout
    VkPipelineLayoutCreateInfo layout_create = {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };

    for (u32 i = 0; i < VULKAN_SHADER_MAX_SETS; i++) {
        if (out_shader->descriptor_sets[i] != VK_NULL_HANDLE) {
            out_shader->set_count++;
        }
    }

    layout_create.setLayoutCount = out_shader->set_count;
    layout_create.pSetLayouts    = out_shader->descriptor_layouts;

    VK_CHECK(vkCreatePipelineLayout(context->logical_device, &layout_create, context->allocator, &out_shader->pipeline_layout));

    VkGraphicsPipelineCreateInfo create_info = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = NULL,
        .stageCount          = stage_count,
        .pStages             = stages,
        .pVertexInputState   = &vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pTessellationState  = &tessellation,
        .pRasterizationState = &rasterization,
        .pMultisampleState   = &multisample,
        .pColorBlendState    = &color_blend,
        .pDepthStencilState  = &depth_state,
        .pViewportState      = &viewport_create,
        .pDynamicState       = &dynamic_state,
        .layout              = out_shader->pipeline_layout,
        .renderPass          = renderpass->handle,
        .subpass             = subpass,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };


    if (use_depth) {
        create_info.pDepthStencilState = &depth_state;
    };

    VK_CHECK(vkCreateGraphicsPipelines(context->logical_device, VK_NULL_HANDLE, 1, &create_info, context->allocator, &out_shader->pipeline));

    // TODO: Push this off onto the materials
    // Update descriptors
    // vulkan_shader_bind_resources(context, out_shader, 0, resource_count, resources);
}

void vulkan_shader_destroy(struct vulkan_context* context, const vulkan_shader_t* shader) {
    vkDestroyShaderModule  (context->logical_device, shader->vert           , context->allocator);
    vkDestroyShaderModule  (context->logical_device, shader->frag           , context->allocator);
    vkDestroyPipelineLayout(context->logical_device, shader->pipeline_layout, context->allocator);
    vkDestroyPipeline      (context->logical_device, shader->pipeline       , context->allocator);

    for (u32 i = 0; i < VULKAN_SHADER_MAX_SETS; i++) {
        if (shader->descriptor_layouts[i] != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(context->logical_device, shader->descriptor_layouts[i], context->allocator);
        }
        if (shader->descriptor_sets[i] != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(context->logical_device, context->descriptor_pool, 1, &shader->descriptor_sets[i]);
        }
    }
}

void vulkan_shader_bind(vulkan_shader_t* shader, vulkan_command_buffer_t* command_buffer) {
    vkCmdBindPipeline(command_buffer->handle, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline);
    vkCmdBindDescriptorSets(command_buffer->handle, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, 
            shader->pipeline_layout,
            0, shader->set_count, shader->descriptor_sets, 
            0, NULL);
}

void create_shader_descriptors(vulkan_context_t* context, u32 set, u32 resource_count, shader_resource_layout_t* resources, vulkan_shader_t* out_shader) {
    SASSERT(resource_count <= VULKAN_SHADER_MAX_RESOURCES, "Cannot create shader descriptor: Too many images. Got %d, max: %d", resource_count, VULKAN_SHADER_MAX_RESOURCES);
    if (resource_count <= 0) {
        SWARN("Cannot create shader descriptors for shader without resources.");
        return;
    }

    // Get count of resource types
    VkDescriptorSetLayoutBinding bindings[VULKAN_SHADER_MAX_RESOURCES];

    u32 binding_count = 0;
    for (u32 i = 0; i < resource_count; i++) {
        if (resources[i].set != set) {
            continue;
        }

        bindings[binding_count].binding         = binding_count;
        bindings[binding_count].descriptorCount = 1;

        switch (resources[i].stage) {
            case SHADER_STAGE_VERTEX:
                bindings[binding_count].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case SHADER_STAGE_FRAGMENT:
                bindings[binding_count].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case SHADER_STAGE_ENUM_MAX:
                break;
        }

        switch (resources[i].type) {
            case SHADER_RESOURCE_SOTRAGE_BUFFER:
                bindings[binding_count].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                break;
            case SHADER_RESOURCE_UNIFORM_BUFFER:
                bindings[binding_count].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                break;
            case SHADER_RESOURCE_SAMPLER:
                bindings[binding_count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                bindings[binding_count].pImmutableSamplers = NULL;
                break;
        }

        binding_count++;
    }

    if (binding_count == 0) {
        return;
    }

    // Create descriptor set layout
    VkDescriptorSetLayoutCreateInfo set_layout_create = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .flags        = 0,
        .bindingCount = binding_count,
        .pBindings    = bindings,
    };

    VK_CHECK(vkCreateDescriptorSetLayout(context->logical_device, &set_layout_create, context->allocator, &out_shader->descriptor_layouts[set]));

    // Allocate descriptor sets
    VkDescriptorSetAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = context->descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &out_shader->descriptor_layouts[set],
    };

    VK_CHECK(vkAllocateDescriptorSets(context->logical_device, &allocate_info, &out_shader->descriptor_sets[set]));
}

void vulkan_shader_bind_resources(struct vulkan_context* context, vulkan_shader_t* shader, u32 binding_start, u32 resource_count, shader_resource_t* resources) {
    // Update descriptor sets
    VkDescriptorBufferInfo buffer_info[VULKAN_SHADER_MAX_RESOURCES];
    VkDescriptorImageInfo image_infos[VULKAN_SHADER_MAX_RESOURCES];

    VkWriteDescriptorSet descriptor_writes[VULKAN_SHADER_MAX_RESOURCES] = {};

    vulkan_image_t* image = NULL;
    for (u32 i = 0; i < resource_count; i++) {
        u32 set = resources[i].layout.set;

        switch (resources[i].layout.type) {
            case VULKAN_BUFFER_TYPE_STORAGE:
                vulkan_buffer_create_descriptor_write(resources[i].data, 
                        resources[i].layout.binding, 
                        shader->descriptor_sets[set], 
                        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
                        &buffer_info[i], 
                        &descriptor_writes[i]);
                break;
            case VULKAN_BUFFER_TYPE_UNIFORM:
                vulkan_buffer_create_descriptor_write(resources[i].data, 
                        resources[i].layout.binding, 
                        shader->descriptor_sets[set], 
                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
                        &buffer_info[i], 
                        &descriptor_writes[i]);
                break;
            case SHADER_RESOURCE_SAMPLER:
                image = resources[i].data;
                image_infos[i].sampler     = image->sampler;
                image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_infos[i].imageView   = image->view;

                descriptor_writes[i] = (VkWriteDescriptorSet) {
                    .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet          = shader->descriptor_sets[set],
                        .dstBinding      = resources[i].layout.binding,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .pImageInfo      = &image_infos[i],
                        .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                };

                break;
        }
    }

    vkUpdateDescriptorSets(context->logical_device, resource_count, descriptor_writes, 0, NULL);
}

