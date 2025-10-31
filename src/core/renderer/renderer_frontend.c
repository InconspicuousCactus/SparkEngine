#include "Spark/renderer/renderer_frontend.h"
#include "Spark/defines.h"
#include "Spark/ecs/ecs_world.h"
#include "Spark/ecs/entity.h"
#include "Spark/math/quat.h"
#include "Spark/math/vec3.h"
#include "Spark/renderer/renderer_types.h"
#include "Spark/renderer/texture.h"
#include "Spark/renderer/vulkan/vulkan_backend.h"
#include "Spark/resources/resource_loader.h"
#include "Spark/resources/resource_types.h"
#include "Spark/resources/resource_functions.h"

struct static_mesh_data;
struct platform_state;

#define SPARK_FORCE_VULKAN

typedef struct render_system_state {
#ifndef SPARK_FORCE_VULKAN
    renderer_backend_t backend;
#endif
    mat4 projection;
    vec3 camera_position;
    f32 near_clip;
    f32 far_clip;
    entity_t skybox;
    material_t* skybox_material;
} render_system_state_t;

static render_system_state_t* state = NULL;
u64 renderer_get_memory_requirements() {
    return sizeof(render_system_state_t);
}

b8 renderer_initialize(const char* application_name, struct platform_state* plat_state, linear_allocator_t* allocator) {
    state = linear_allocator_allocate(allocator, sizeof(render_system_state_t));

    // Choose renderer type
#ifndef SPARK_FORCE_VULKAN
    renderer_backend_type_t backend = RENDERER_BACKEND_VULKAN;
    switch (backend) {
    case RENDERER_BACKEND_VULKAN:
        vulkan_renderer_initialize(&state_ptr->backend, application_name, plat_state, allocator);

        state_ptr->backend = 
            (renderer_backend_t) {
                .initialize             = vulkan_renderer_initialize,
                .shutdown               = vulkan_renderer_shutdown,
                .begin_draw_frame       = vulkan_renderer_begin_frame,
                .draw_frame             = vulkan_renderer_draw_frame,
                .end_draw_frame         = vulkan_renderer_end_frame,
                .create_mesh            = vulkan_create_mesh,
                .get_screen_size        = vulkan_get_screen_size,
                .creaet_shader          = vulkan_create_shader,
                .create_image_from_file = vulkan_create_image_from_file,
                .create_image_from_data = vulkan_create_image_from_data,
                .create_material        = vulkan_create_material,
                .resize                 = vulkan_renderer_resize,
                .material_update_buffer = vulkan_renderer_material_update_buffer,
            };

        break;
    default:
        SCRITICAL("Unimplemented renderer type. Shutting down.");
        break;
    }
#else
    vulkan_renderer_initialize(application_name, plat_state, allocator);
#endif

    // Setup skybox
    // const static u32 max_vertices = 256 * 6;
    // const static u32 max_indices = 1024 * 6;
    // vec3 vertices[max_vertices] = {};
    // u32 indices[max_indices] = {};
    // u32 index_count = 0;
    // u32 vertex_count = 0;
    // create_sphere(6, max_vertices, max_indices, vertices, indices, &vertex_count, &index_count);
    // mesh_t skybox_mesh = renderer_create_mesh(vertices, vertex_count, sizeof(vec3), indices, index_count, sizeof(u32));

    // ecs_world_t* world = ecs_world_get();
    // state->skybox = entity_create(world);
    // const static aabb_t skybox_aabb = {
    //     .center = { },
    //     .extents = { .x = 1, .y = 1, .z = 1 },
    // };
    // entity_add_transforms(world, state->skybox, vec3_zero(), vec3_one(), quat_identity());
    // // ENTITY_SET_COMPONENT(world, state->skybox, local_to_world_t, {});
    // ENTITY_SET_COMPONENT(world, state->skybox, mesh_t, skybox_mesh);
    // ENTITY_SET_COMPONENT(world, state->skybox, aabb_t, skybox_aabb);

    return true;
}

#ifdef SPARK_FORCE_VULKAN
void renderer_shutdown() {
    vulkan_renderer_shutdown();
}

void renderer_on_resize(u16 width, u16 height) {
    vulkan_renderer_resize(width, height);
}

vec2 renderer_get_screen_size() {
    return vulkan_get_screen_size();
}

b8 renderer_draw_frame(render_packet_t* packet) {
    vulkan_renderer_begin_frame();
    vulkan_renderer_draw_frame(packet);
    vulkan_renderer_end_frame();
    return true;
}

mesh_t renderer_create_mesh(const void* vertices, u32 vertex_count, u32 vertex_stride, const void* indices, u32 index_count, u32 index_stride) {
    if (index_count <= 0 || vertex_count <= 0) {
        return (mesh_t) {
            .internal_index = INVALID_ID,
        };
    }
    SASSERT(vertex_count > 0, "Cannot create a mesh without vertices");
    return vulkan_create_mesh(vertices, vertex_count, vertex_stride, indices, index_count, index_stride);
}

shader_t renderer_create_shader(shader_config_t* config) {
    return vulkan_create_shader(config);
}

texture_t renderer_create_image_from_path(const char* path, texture_filter_t filter) {
    return vulkan_create_image_from_file(path, filter);
}

texture_t renderer_create_image_from_data(const char* data, u32 width, u32 height, u32 channels, texture_filter_t filter) {
    return vulkan_create_image_from_data(data, width, height, channels, filter);
}

material_t renderer_create_material(material_config_t* config) {
    return vulkan_create_material(config);
}

void material_update_buffer(material_t* material, void* data, u32 size, u32 offset) {
    return vulkan_renderer_material_update_buffer(material, data, size, offset);
}

#else
void renderer_shutdown() {
    state_ptr->backend.shutdown(&state_ptr->backend);
}

void renderer_on_resize(u16 width, u16 height) {
    state_ptr->backend.resize(width, height);
}

vec2 renderer_get_screen_size() {
    return state_ptr->backend.get_screen_size();
}

b8 renderer_draw_frame(render_packet_t* packet) {
    state_ptr->backend.begin_draw_frame();
    state_ptr->backend.draw_frame(packet);
    state_ptr->backend.end_draw_frame();
    return true;
}


mesh_t renderer_create_mesh(void* vertices, u32 vertex_count, u32 vertex_stride, void* indices, u32 index_count, u32 index_stride) {
    SASSERT(vertex_count > 0, "Cannot create a mesh without vertices");
    return state_ptr->backend.create_mesh(vertices, vertex_count, vertex_stride, indices, index_count, index_stride);
}

shader_t renderer_create_shader(shader_config_t* config) {
    return state_ptr->backend.creaet_shader(config);
}

texture_t renderer_create_image_from_path(const char* path, texture_filter_t filter) {
    return state_ptr->backend.create_image_from_file(path, filter);
}

texture_t renderer_create_image_from_data(const char* data, u32 width, u32 height, u32 channels, texture_filter_t filter) {
    return state_ptr->backend.create_image_from_data(data, width, height, channels, filter);
}

material_t renderer_create_material(material_config_t* config) {
    return state_ptr->backend.create_material(config);
}

void material_update_buffer(material_t* material, void* data, u32 size, u32 offset) {
    state_ptr->backend.material_update_buffer(material, data, size, offset);
}
#endif

void renderer_set_skybox(material_t* material) {
    ecs_world_t* world = ecs_world_get();
    resource_t skybox_res = resource_loader_get_resource("assets/resources/models/skybox", false);
    state->skybox = resource_instance_model(&skybox_res, 0, &material);
    state->skybox_material = material;
    ENTITY_SET_COMPONENT(world, state->skybox, material_t, *state->skybox_material);
}
