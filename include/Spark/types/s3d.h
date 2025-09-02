#pragma once
#include "Spark/defines.h"
#include "Spark/math/math_types.h"
#include "Spark/types/aabb.h"

typedef enum vertex_attributes : u16 {
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_POSITION_3D = 0x1,
    VERTEX_ATTRIBUTE_POSITION_2D = 0x2,
    VERTEX_ATTRIBUTE_NORMAL      = 0x4,
    VERTEX_ATTRIBUTE_COLOR0      = 0x8,
    VERTEX_ATTRIBUTE_COLOR1      = 0x10,
    VERTEX_ATTRIBUTE_COLOR2      = 0x20,
    VERTEX_ATTRIBUTE_COLOR3      = 0x40,
    VERTEX_ATTRIBUTE_UV0         = 0x80,
    VERTEX_ATTRIBUTE_UV1         = 0x100,
    VERTEX_ATTRIBUTE_UV2         = 0x200,
    VERTEX_ATTRIBUTE_UV3         = 0x400,
    VERTEX_ATTRIBUTE_INT         = 0x800,
} vertex_attributes_t;

typedef enum s3d_texture_type : u8 {
    S3D_TEXTURE_TYPE_INVALID,
    S3D_TEXTURE_TYPE_DIFFUSE,
} s3d_texture_type_t;

typedef struct s3d_texture {
    u32 data_length;
    u32 data_offset;
} s3d_texture_t;

typedef struct s3d_material {
    u8 texture_count;
    u8 texture_indices[7];
} s3d_material_t;

#define S3D_FILE_MAGIC 0x5F533344
typedef struct s3d_mesh {
    aabb_t bounds;

    u32 vertex_offset;
    u32 vertex_count;

    // NOTE: Set in this order for struct packing reasons
    u8 vertex_stride;
    u8 index_stride;

    u32 index_offset;
    u32 index_count;

    u8 material_index;
    vertex_attributes_t attributes;
} s3d_mesh_t;

typedef struct s3d_object {
    vec3 position;
    quat rotation;
    vec3 scale;
    u16 child_count;
    u16 mesh_index;
    u32 parent_index;
} s3d_object_t;

typedef struct s3d {
    u32 file_magic;

    u32 object_count;
    u32 mesh_offset;
    /**
     * @brief Packed array of all mesh vertices (may contain different formats)
     */
    u32 vertex_offset;
    /**
     * @brief Packed array of all mesh indices (may contain different formats)
     */
    u32 index_offset;

    u16 material_count;
    u32 material_offset;

    u32 texture_offset;
    u32 texture_count;
} s3d_t;
