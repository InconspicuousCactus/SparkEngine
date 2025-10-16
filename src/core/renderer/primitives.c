#include "Spark/renderer/primitives.h"
#include "Spark/math/math_types.h"
#include "Spark/math/vec3.h"

SINLINE void wind_quad(u32* indices, u32 index_offset, u32 vertex_offset, u32 edge_vertex_count, b8 invert) {
    if (!invert) {
        indices[index_offset + 0] = vertex_offset + 0;
        indices[index_offset + 1] = vertex_offset + 1;
        indices[index_offset + 2] = vertex_offset + edge_vertex_count;

        indices[index_offset + 3] = vertex_offset + 1;
        indices[index_offset + 4] = vertex_offset + 1 + edge_vertex_count;
        indices[index_offset + 5] = vertex_offset + edge_vertex_count;
    } else {
        indices[index_offset + 0] = vertex_offset + 0;
        indices[index_offset + 1] = vertex_offset + edge_vertex_count;
        indices[index_offset + 2] = vertex_offset + 1;

        indices[index_offset + 3] = vertex_offset + 1;
        indices[index_offset + 4] = vertex_offset + edge_vertex_count;
        indices[index_offset + 5] = vertex_offset + 1 + edge_vertex_count;
    }
}

SINLINE void create_plane(u32 plane_index, u32 resolution, vec3* out_vertices, u32* out_indices, u32 vertex_offset, u32 index_offset) {
    for (u32 y = 0, vertex = vertex_offset, index = index_offset; y < resolution; y++) {
        for (u32 x = 0; x < resolution; x++, vertex++) {
            // Front / Back
            vec3 pos;
            switch (plane_index) {
                default:
                case 0:
                    pos = vec3_normalized(
                    (vec3) {
                    .x = (f32)x / (resolution - 1) * 2.f - 1.0f,
                    .y = (f32)y / (resolution - 1) * 2.f - 1.0f,
                    .z = 1,
                    });
                    break;
                case 1: 
                    pos = vec3_normalized(
                    (vec3) {
                    .x = (f32)x / (resolution - 1) * 2.f - 1.0f,
                    .y = (f32)y / (resolution - 1) * 2.f - 1.0f,
                    .z = -1,
                    });
                    break;
                case 2:
                    pos = vec3_normalized(
                    (vec3) {
                    .x = (f32)x / (resolution - 1) * 2.f - 1.0f,
                    .z = (f32)y / (resolution - 1) * 2.f - 1.0f,
                    .y = 1,
                    });
                    break;
                case 3:
                    pos = vec3_normalized(
                    (vec3) {
                    .x = (f32)x / (resolution - 1) * 2.f - 1.0f,
                    .z = (f32)y / (resolution - 1) * 2.f - 1.0f,
                    .y = -1,
                    });
                    break;
                case 4:
                    pos = vec3_normalized(
                    (vec3) {
                    .z = (f32)x / (resolution - 1) * 2.f - 1.0f,
                    .y = (f32)y / (resolution - 1) * 2.f - 1.0f,
                    .x = 1,
                    });
                    break;
                case 5:
                    pos = vec3_normalized(
                    (vec3) {
                    .z = (f32)x / (resolution - 1) * 2.f - 1.0f,
                    .y = (f32)y / (resolution - 1) * 2.f - 1.0f,
                    .x = -1,
                    });
                    break;
                    
            }

            out_vertices[vertex] = pos;
            b8 invert = false;
            if (plane_index == 2 || plane_index == 4 || plane_index == 1) {
                invert = true;
            }
            if (x != resolution - 1 && y != resolution - 1) {
                if (invert) {
                    out_indices[index + 0] = vertex; 
                    out_indices[index + 1] = vertex + resolution + 1;
                    out_indices[index + 2] = vertex + resolution; 

                    out_indices[index + 3] = vertex; 
                    out_indices[index + 4] = vertex + 1;
                    out_indices[index + 5] = vertex + resolution + 1;
                } else {
                    out_indices[index + 0] = vertex; 
                    out_indices[index + 1] = vertex + resolution; 
                    out_indices[index + 2] = vertex + resolution + 1;

                    out_indices[index + 3] = vertex; 
                    out_indices[index + 4] = vertex + resolution + 1;
                    out_indices[index + 5] = vertex + 1;
                }
                index += 6;
            }
        }
    }
}

void create_sphere(u32 resolution, u32 max_vertex_count, u32 max_index_count, vec3* out_vertices, u32* out_indices, u32* out_vertex_count, u32* out_index_count) {
    u32 vertex_count = resolution * resolution;
    u32 index_count = (resolution - 1) * (resolution - 1) * 6;

    *out_vertex_count = vertex_count * 6;
    *out_index_count = index_count * 6;

    SASSERT(vertex_count * 6 < max_vertex_count, "Not enough space allocated to create sphere vertices. Creating %d vertices when max is %d", vertex_count * 6, max_vertex_count);
    SASSERT(index_count * 6 < max_index_count, "Not enough space allocated to create sphere indices. Creating %d indices when max is %d", index_count, max_index_count);

    create_plane(0, resolution, out_vertices, out_indices, vertex_count * 0, index_count * 0);
    create_plane(1, resolution, out_vertices, out_indices, vertex_count * 1, index_count * 1);
    create_plane(2, resolution, out_vertices, out_indices, vertex_count * 2, index_count * 2);
    create_plane(3, resolution, out_vertices, out_indices, vertex_count * 3, index_count * 3);
    create_plane(4, resolution, out_vertices, out_indices, vertex_count * 4, index_count * 4);
    create_plane(5, resolution, out_vertices, out_indices, vertex_count * 5, index_count * 5);
}
