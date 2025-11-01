#pragma once

#include "Spark/ecs/entity.h"
#include "Spark/renderer/texture.h"

typedef enum resource_type : u8 {
    RESOURCE_TYPE_NULL,
    RESOURCE_TYPE_TEXT,
    RESOURCE_TYPE_MODEL,
    RESOURCE_TYPE_MESH,
    RESOURCE_TYPE_SHADER,
    RESOURCE_TYPE_TEXTURE,
    RESOURCE_TYPE_MATERIAL,
    RESOURCE_TYPE_ENUM_MAX,
} resource_type_t;

const static u32 BINARY_RESOURCE_FILE_MAGIC = 0x99 | ('b' << 8) | ('r' << 16) | ('s' << 24);
typedef struct binary_resource_header {
    u32 magic;
    resource_type_t type;
} binary_resource_header_t;

typedef struct resource {
    resource_type_t type;
    b8 auto_delete;
    u8 generation;
    u16 ref_count;
    u16 internal_index;
} resource_t;
darray_header(resource_t, resource);

typedef struct mesh_resource {
    u32 mesh_index;
} mesh_resource_t;

// Configurations

#define TEXTURE_CONFIG_PATH_MAX_SIZE 128
typedef struct texture_config {
    char path[TEXTURE_CONFIG_PATH_MAX_SIZE];
    texture_filter_t filter;
} texture_config_t;

struct texture;
struct shader;
struct material;

struct texture*  resource_get_texture(resource_t* resource);
struct shader*   resource_get_shader(resource_t* resource);
struct material* resource_get_material(resource_t* resource);
entity_t    resource_instance_model(resource_t* resource, u32 material_count, struct material** materials);
