#pragma once

#include "Spark/ecs/ecs.h"
#include "Spark/renderer/shader.h"
#include "Spark/renderer/texture.h"

typedef struct material_resource {
    shader_resource_type_t type;
    u16 set;
    u16 binding;
    void* value;
} material_resource_t;

#define MATERIAL_MAX_RESOURCE_COUNT 8
#define MATERIAL_CONFIG_MAX_NAME_LENGTH 48
typedef struct material {
#ifdef SPARK_DEBUG
    char name[MATERIAL_CONFIG_MAX_NAME_LENGTH];
#endif 
    u16 internal_index;
    u16 shader_index;
} material_t;
extern ECS_COMPONENT_DECLARE(material_t);

typedef struct material_config {
    char name[MATERIAL_CONFIG_MAX_NAME_LENGTH];
    char shader_path[MATERIAL_CONFIG_MAX_NAME_LENGTH];
    u8 resource_count;
    material_resource_t resources[MATERIAL_MAX_RESOURCE_COUNT];
} material_config_t;

void material_set_texture(material_t* material, texture_t* texture, u32 set, u32 binding);
void material_update(material_t* update);
void material_update_buffer(material_t* material, void* data, u32 size, u32 offset);
