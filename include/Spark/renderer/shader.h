#pragma once

#include "Spark/renderer/renderpasses.h"
#include "Spark/resources/resource_types.h"
#include "Spark/types/s3d.h"
#include "Spark/containers/darray.h"

#define SHADER_MAX_VERTEX_ATTRIBUTES 8

// NOTE: if extra stages are added, ensure that SHADER_STAGE_ENUM_MAX fits within shader_resource_layout_t's stage bitfield
typedef enum shader_stage_flags : u8 {
    SHADER_STAGE_NONE = 0,
    SHADER_STAGE_VERTEX = 1,
    SHADER_STAGE_FRAGMENT = 2,
    SHADER_STAGE_ENUM_MAX,
} shader_stage_flags_t;

// NOTE: if extra stages are added, ensure that SHADER_RESOURCE_ENUM_MAX fits within shader_resource_layout_t's type bitfield
typedef enum shader_resource_type : u8 {
    SHADER_RESOURCE_UNDEFINED,
    SHADER_RESOURCE_SOTRAGE_BUFFER,
    SHADER_RESOURCE_UNIFORM_BUFFER,
    SHADER_RESOURCE_SAMPLER,
} shader_resource_type_t;

typedef struct shader_resource_layout {
    shader_resource_type_t type : 8; 
    u8 set : 4;
    u8 binding : 4;
} shader_resource_layout_t;

typedef struct shader_resource {
    void* data;
    shader_resource_layout_t layout;
} shader_resource_t;

#define SHADER_MAX_RESOURCES 8
#define SHADER_MAX_ATTRIBUTES 8
typedef struct shader {
    u32 internal_index;
    shader_resource_layout_t layout[SHADER_MAX_RESOURCES];
    vertex_attributes_t attributes[SHADER_MAX_ATTRIBUTES];
    builtin_renderpass_t type;
    u8 resource_count;
    u8 attribute_count;
#ifdef SPARK_DEBUG
    char name[64];
#endif
} shader_t;

darray_header(shader_t, shader);

#define SHADER_CONFIG_MAX_NAME_LENGTH 16
typedef struct shader_config {
    char name[SHADER_CONFIG_MAX_NAME_LENGTH];
    const void* vertex_spv;
    const void* fragment_spv;
    u32 vertex_spv_size;
    u32 fragment_spv_size;
    shader_resource_layout_t layout[SHADER_MAX_RESOURCES];
    vertex_attributes_t attributes[SHADER_MAX_ATTRIBUTES];
    u8 resource_count;
    u8 attribute_count;
    builtin_renderpass_t type;
} shader_config_t;

typedef struct binary_shader_resource {
    binary_resource_header_t header;
    shader_config_t config;
} binary_shader_resource_t;
