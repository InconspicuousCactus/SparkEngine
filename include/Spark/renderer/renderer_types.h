#pragma once
#include "Spark/renderer/renderpasses.h"
#include "Spark/defines.h"
#include "Spark/math/mat4.h"
#include "Spark/math/math_types.h"
#include "Spark/memory/linear_allocator.h"
#include "Spark/renderer/material.h"
#include "Spark/renderer/mesh.h"
#include "Spark/renderer/texture.h"

typedef enum {
    RENDERER_BACKEND_GLFW,
    RENDERER_BACKEND_VULKAN,
    RENDERER_BACKEND_DIRECTX,
} renderer_backend_type_t;

typedef struct geometry_render_data {
    mat4 model;
    mesh_t mesh;
    material_t* material;
    vec3 position;
} geometry_render_data_t;

typedef struct renderpass_geometry {
    u32 geometry_count;
    geometry_render_data_t* geometry;
} renderpass_geometry_t;

typedef struct render_packet {
    f32 delta_time;

    vec3 view_pos;
    quat view_rotation;
    // mat4 view_matrix;
    mat4 projection_matrix;

    /**
     * @brief Indexed via builtin_renderpass enum
     */
    renderpass_geometry_t renderpass_geometry[BUILTIN_RENDERPASS_ENUM_MAX];
} render_packet_t;

typedef struct renderer_backend {
    struct platform_state* plat_state;

    b8 (*initialize)(struct renderer_backend* backend, const char* application_name, struct platform_state* plat_state, linear_allocator_t* allocator);
    b8 (*shutdown)(struct renderer_backend* backend);

    b8 (*begin_draw_frame)();
    b8 (*draw_frame)(render_packet_t* packet);
    b8 (*end_draw_frame)();

    vec2 (*get_screen_size)();
    void (*resize)(u16 width, u16 height);

    // Type Creation
    mesh_t (*create_mesh)(void* vertices, u32 vertex_count, u32 vertex_stride, void* indices, u32 index_count, u32 index_stride);
    shader_t (*creaet_shader)(shader_config_t* config);
    texture_t (*create_image_from_file)(const char* path, texture_filter_t filter);
    texture_t (*create_image_from_data)(const char* data, u32 widht, u32 height, u32 channels, texture_filter_t filter);
    material_t (*create_material)(material_config_t* config);

    // Resource specific functions
    // Materials
    void (*material_set_buffer)(material_t* material, void* data, u32 size, u32 offset);

    /**
     * @brief The number of frames rendered
     */
    u64 frame_count;

} renderer_backend_t;

void register_render_types();
