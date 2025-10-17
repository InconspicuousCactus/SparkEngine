#include "Spark/resources/loaders/material_loader.h"
#include "Spark/core/sstring.h"
#include "Spark/memory/linear_allocator.h"
#include "Spark/renderer/material.h"
#include "Spark/renderer/renderer_frontend.h"
#include "Spark/renderer/shader.h"
#include "Spark/resources/loaders/loader_utils.h"
#include "Spark/resources/resource_loader.h"
#include "Spark/resources/resource_types.h"
#include "Spark/resources/resource_functions.h"

// ==============================
// Private Data
// ==============================
darray_type(material_t, material);
darray_impl(material_t, material);

typedef struct material_loader_state {
    darray_material_t materials;
} material_loader_state_t;
static material_loader_state_t* loader_state;

// ==============================
// Private Function Definitions
// ==============================

void material_loader_initialzie(linear_allocator_t* allocator) {
    loader_state = linear_allocator_allocate(allocator, sizeof(material_loader_state_t));
    darray_material_create(32, &loader_state->materials);
}
void material_loader_shutdown() {
    darray_material_destroy(&loader_state->materials);
}

resource_t material_loader_load_text_resouce(const char* text, u32 length, b8 auto_delete) {
    // Create resource info
    u32 material_index = loader_state->materials.count;
    resource_t resource = {
        .internal_index = material_index,
        .type = RESOURCE_TYPE_MATERIAL,
        .auto_delete = auto_delete,
        .generation = 0,
        .ref_count = 1,
    };

    material_config_t config = {};

    const u32 arg_buffer_count = 8;
    const u32 arg_buffer_size = 48;
    char args[arg_buffer_count][arg_buffer_size];
    u32 text_offset = 0;

    while (text_offset < length) {
        u32 arg_count = 0;
        loader_read_line(text, &text_offset, (char*)args, arg_buffer_count, arg_buffer_size, &arg_count);

        if (string_equal(args[0], "name")) {
            string_copy(config.name, args[1]);
        } else if (string_equal(args[0], "shader_path")) {
            string_ncopy(config.shader_path, args[1], MATERIAL_CONFIG_MAX_NAME_LENGTH);
        } else if (string_equal(args[0], "texture")) {
            SASSERT(arg_count == 4, "Cannot load material texture: Invalid format, expected 4 args. Format: 'texture:path,binding,set'");
            u32 resource_index = config.resource_count++;
            SASSERT(resource_index < SHADER_MAX_RESOURCES, "Cannot add more than %d resources from shader resource.", SHADER_MAX_RESOURCES);

            // Get texture
            resource_t texture_resource = resource_loader_get_resource(args[1], false);
            texture_t* texture = resource_get_texture(&texture_resource);

            u32 binding = 0;
            u32 set = 0;

            string_to_u32(args[2], &binding);
            string_to_u32(args[3], &set);
            
            config.resources[resource_index].type = SHADER_RESOURCE_SAMPLER;
            config.resources[resource_index].binding = binding;
            config.resources[resource_index].set = set;
            config.resources[resource_index].value = texture;
        } else if (string_equal(args[0], "storage_buffer")) {
            SASSERT(arg_count == 3, "Cannot load material buffer: Invalid format, expected 3 args. Format: 'texture:binding,set'");
            u32 resource_index = config.resource_count++;
            SASSERT(resource_index < SHADER_MAX_RESOURCES, "Cannot add more than %d resources from shader resource.", SHADER_MAX_RESOURCES);

            // Get texture
            u32 binding = 0;
            u32 set = 0;

            string_to_u32(args[1], &binding);
            string_to_u32(args[2], &set);
            
            config.resources[resource_index].type = SHADER_RESOURCE_SOTRAGE_BUFFER;
            config.resources[resource_index].binding = binding;
            config.resources[resource_index].set = set;
            // config.resources[resource_index].value = texture;
        } else {
            SWARN("Failed to get shader resource key: '%s'", args[0]);
        }
    }

    material_loader_create_from_config(&config);
    return resource;
}

// ==============================
// Private Function Implementations
// ==============================
material_t* material_loader_create_from_config(material_config_t* config) {
    material_t mat = renderer_create_material(config);
    return darray_material_push(&loader_state->materials, mat);
}

material_t* material_loader_get_material(u32 index) {
    SASSERT(index < loader_state->materials.count && index >= 0, "Cannot load shader with out of bounds index. Index: %d, Shader Count: %d", index, loader_state->materials.count);
    return &loader_state->materials.data[index];
}
