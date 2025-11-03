#include "Spark/ui/text.h"
#include "Spark/core/sstring.h"
#include "Spark/ecs/entity.h"
#include "Spark/math/mat4.h"
#include "Spark/renderer/mesh.h"
#include "Spark/renderer/renderer_frontend.h"
#include "Spark/types/transforms.h"
#include "Spark/ui/anchor_2d.h"
#include "Spark/ui/ui_defaults.h"

entity_t text_create(ecs_world_t* world, text_t text, vec2 anchor) {
    entity_t e = entity_create(world);
    ENTITY_SET_COMPONENT(world, e, text_t, text);
        // ECS_COMPONENT_ID(local_to_world_t), 
        // ECS_COMPONENT_ID(aabb_t), 
        // ECS_COMPONENT_ID(material_t)
    ENTITY_SET_COMPONENT(world, e, local_to_world_t, {mat4_identity()});
    ENTITY_SET_COMPONENT(world, e, aabb_t, {.extents = { 1, 1, 1} });
    ENTITY_SET_COMPONENT(world, e, material_t, *ui_defaults.text_material);
    ENTITY_SET_COMPONENT(world, e, anchor_2d_t, { .pos = anchor });

    text_update(world, e, text);

    return e;
}

void text_update(ecs_world_t* world, entity_t entity, text_t text) {
    // Try to delete old mesh
    mesh_t* mesh = NULL;
    if (ENTITY_TRY_GET_COMPONENT(world, entity, mesh_t, &mesh)) {
        SDEBUG("Freeing old text mesh.");
        renderer_destroy_mesh(mesh);
    }

    // Create new mesh
    constexpr u32 max_vertex_count = 8192;
    static vertex_2d vertices[max_vertex_count] = {};

    constexpr u32 max_index_count = 8192 * 3;
    static u32 indices[max_index_count] = {};

    const u32 text_len = string_length(text.value);
    f32 x_offset = 0;
    f32 y_offset = 0;

    const f32 x_step = text.point_size;
    const f32 y_step = text.point_size;

    const f32 char_size = text.point_size;

    u32 vertex_count = 0;
    u32 index_count = 0;
    for (u32 i = 0; i < text_len; i++) {
        char c = text.value[i];
        if (c == '\n') {
            x_offset = 0;
            y_offset += y_step;
            continue;
        }
        if (c == '\t') {
            x_offset += x_offset * 5;
            continue;
        }

        constexpr f32 uv_scale = 1.0f / 16.0f;
        vec2 uv_base = {
            .x = (f32)(c % 16) / 16.0f,
            .y = (f32)(c / 16) / 16.0f,
        };

        vertices[vertex_count + 0] = (vertex_2d) {
                .position = {
                    .x = 0 + x_offset,
                    .y = 0 + y_offset,
                },
                .texcoord = {
                    .x = uv_base.x + 0,
                    .y = uv_base.y + 0,
                },
            };

        vertices[vertex_count + 1] = (vertex_2d) {
                .position = {
                    .x = 0 + x_offset,
                    .y = char_size + y_offset,
                },
                .texcoord = {
                    .x = uv_base.x + 0,
                    .y = uv_base.y + uv_scale,
                },
            };

        vertices[vertex_count + 2] = (vertex_2d) {
                .position = {
                    .x = char_size + x_offset,
                    .y = 0 + y_offset,
                },
                .texcoord = {
                    .x = uv_base.x + uv_scale,
                    .y = uv_base.y + 0,
                },
            };

        vertices[vertex_count + 3] = (vertex_2d) {
                .position = {
                    .x = char_size + x_offset,
                    .y = char_size + y_offset,
                },
                .texcoord = {
                    .x = uv_base.x + uv_scale,
                    .y = uv_base.y + uv_scale,
                },
            };

        indices[index_count + 0] = vertex_count + 0;
        indices[index_count + 1] = vertex_count + 2;
        indices[index_count + 2] = vertex_count + 1;

        indices[index_count + 3] = vertex_count + 2;
        indices[index_count + 4] = vertex_count + 3;
        indices[index_count + 5] = vertex_count + 1;

        x_offset += x_step;
        vertex_count += 4; 
        index_count += 6;
    }

    mesh_t new_mesh = renderer_create_mesh(vertices, vertex_count, sizeof(vertex_2d), indices, index_count, sizeof(u32));
    ENTITY_SET_COMPONENT(world, entity, mesh_t, new_mesh);
}
