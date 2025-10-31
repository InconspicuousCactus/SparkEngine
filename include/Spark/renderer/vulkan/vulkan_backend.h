#pragma once
#include "Spark/defines.h"
#include "Spark/memory/linear_allocator.h"
#include "Spark/renderer/material.h"
#include "Spark/renderer/renderer_types.h"

b8 vulkan_renderer_initialize(const char* application_name, struct platform_state* plat_state, linear_allocator_t* allocator);
b8 vulkan_renderer_shutdown();

void vulkan_renderer_resize(u16 widhh, u16 height);

b8 vulkan_renderer_begin_frame();
b8 vulkan_renderer_draw_frame(render_packet_t* packet);
b8 vulkan_renderer_end_frame();

mesh_t     vulkan_create_mesh(const void* vertices, u32 vertex_count, u32 vertex_size, const void* indices, u32 index_count, u32 index_stride);
shader_t   vulkan_create_shader(shader_config_t* config);
material_t vulkan_create_material(material_config_t* config);

texture_t vulkan_create_image_from_file(const char* path, texture_filter_t filter);
texture_t vulkan_create_image_from_data(const char* data, u32 width, u32 height, u32 channels, texture_filter_t filter);

vec2 vulkan_get_screen_size();

void vulkan_renderer_material_update_buffer(material_t* material, void* data, u32 size, u32 offset);
