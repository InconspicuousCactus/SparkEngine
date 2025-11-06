#pragma once
#include "Spark/renderer/mesh.h"
#include "Spark/renderer/renderer_types.h"
#include "Spark/memory/linear_allocator.h"

struct static_mesh_data;
struct platform_state;

b8   renderer_initialize(const char* application_name, struct platform_state* plat_state, linear_allocator_t* allocator);
void renderer_shutdown();

b8   renderer_draw_frame(render_packet_t* packet);
void renderer_on_resize(u16 width, u16 height);

mesh_t     renderer_create_mesh           (const void* vertices, u32 vertex_count, u32 vertex_stride, const void* indices, u32 index_count, u32 index_stride);
void       renderer_destroy_mesh          (const mesh_t* mesh);
shader_t   renderer_create_shader         (shader_config_t* config);
texture_t  renderer_create_image_from_path(const char* path, texture_filter_t filter);
texture_t  renderer_create_image_from_data(const char* data, u32 width, u32 height, u32 channels, texture_filter_t filter);
material_t renderer_create_material       (material_config_t* config);

const renderer_defaults_t renderer_get_default_types();

void renderer_set_skybox(material_t* material);

vec2 renderer_get_screen_size();

