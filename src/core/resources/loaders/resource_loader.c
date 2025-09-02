#include "Spark/resources/resource_loader.h"
#include "Spark/containers/generic/darray_ints.h"
#include "Spark/containers/unordered_map.h"
#include "Spark/core/logging.h"
#include "Spark/core/sstring.h"
#include "Spark/defines.h"
#include "Spark/memory/linear_allocator.h"
#include "Spark/platform/filesystem.h"
#include "Spark/resources/loaders/model_loader.h"
#include "Spark/resources/resource_types.h"
#include "Spark/resources/loaders/material_loader.h"
#include "Spark/resources/loaders/shader_loader.h"
#include "Spark/resources/loaders/texture_loader.h"

// =================================
// Private Data Types
// =================================
#define TRES_TAG "#resource"
#define TRES_SIZE sizeof(TRES_TAG)
darray_impl(resource_t, resource);

darray_header(darray_resource_t, resource_set);

hashmap_type(resource_map, const char*, resource_t);
hashmap_impl(resource_map, const char*, resource_t, string_hash, string_compare);

typedef struct shader_loader_state {
    darray_u8_t resource_buffer;
    resource_map_t resources;
} resource_loader_state_t;

static resource_loader_state_t* loader_state;

// =================================
// Private Function Definitions 
// =================================
void load_binary_resource(const char* data, u32 size, b8 auto_delete, resource_t* out_resource);
#define is_tres_type(text, type) (string_nequal(text + sizeof(TRES_TAG), type, sizeof(type) - 1))

// =================================
// Function Implementations
// =================================
void resource_loader_initialize(linear_allocator_t* allocator) {
    loader_state = linear_allocator_allocate(allocator, sizeof(resource_loader_state_t));
    darray_u8_create(256, &loader_state->resource_buffer);
    resource_map_create(1024, &loader_state->resources);

    // Load individual loaders
    shader_loader_initialzie(allocator);
    texture_loader_initialzie(allocator);
    material_loader_initialzie(allocator);
    model_loader_initialzie(allocator);
}
void resource_loader_shutdown() {
    // Shutdown loaders
    shader_loader_shutdown();
    texture_loader_shutdown();
    material_loader_shutdown();
    model_loader_shutdown();

    // Destroy private data
    // resource_map_destroy(&loader_state->resources);
    // darray_u8_destroy(&loader_state->resource_buffer);
}

resource_t resource_loader_get_resource(const char* path, b8 auto_delete) {
    resource_t out_resource = (resource_t) { .type = RESOURCE_TYPE_NULL };
    if (resource_map_try_get(&loader_state->resources, path, &out_resource)) {
        return out_resource;
    }

    // Load file
    // Load the bytes
    file_handle_t file_handle;
    b8 opened_file = filesystem_open(path, FILE_MODE_READ, true, &file_handle);
    SASSERT(opened_file, "Failed to open resource at path '%s'", path);

    u64 file_size = 0;
    filesystem_size(&file_handle, &file_size);

    u64 bytes_read = 0;
    filesystem_read(&file_handle, file_size, loader_state->resource_buffer.data, &bytes_read);
    filesystem_close(&file_handle);
    loader_state->resource_buffer.data[bytes_read] = file_size;

    // Check if the resource is a binary or text resource
    if (((u16*)loader_state->resource_buffer.data)[0] == BINARY_RESOURCE_FILE_MAGIC) {
        load_binary_resource((const char*)loader_state->resource_buffer.data, file_size, auto_delete, &out_resource);
    } else {
        if (!string_nequal((const char*)loader_state->resource_buffer.data, TRES_TAG, TRES_SIZE - 1)) {
            SERROR("Cannot load resource at path '%s': Text resource must have first line defining resource type. I.e. '#resource shader'", path);
            return (resource_t) { .type = RESOURCE_TYPE_NULL };
        }

        // Check for text resource
        // TODO: Remove strncmp, Should be custom string library
        SASSERT(string_equal((const char*)loader_state->resource_buffer.data, "#resource shader") == 0, 
                "Trying to load binary shader resource. Expected resource tag of '#resource shader', got '%.16s'",
                (const char*)loader_state->resource_buffer.data);
        const char* resource_text = (const char*)loader_state->resource_buffer.data;

        if (is_tres_type(resource_text, "shader")) {
            out_resource = pvt_shader_loader_load_text_resource((const char*)loader_state->resource_buffer.data, file_size, auto_delete);
        } else if (is_tres_type(resource_text, "material")) {
            out_resource = material_loader_load_text_resouce(resource_text, file_size, auto_delete);
        } else if (is_tres_type(resource_text, "texture")) {
            out_resource = texture_loader_load_text_resouce(resource_text, file_size, auto_delete);
        } else if (is_tres_type(resource_text, "model")) {
            out_resource = pvt_model_loader_load_text_resource(resource_text, file_size, auto_delete);
        } else{
            SERROR("Failed to get text resource type");
            return (resource_t) { .type = RESOURCE_TYPE_NULL };
        }

        resource_map_insert(&loader_state->resources, path, out_resource);

    }

    return out_resource;
}
void resource_loader_destroy_resource(resource_t* resource) {
    // Just set index to invalid and increment the generation
}

mesh_resource_t resource_loader_load_mesh(const char* path);

// =================================
// Private Function Implementations
// =================================
void load_binary_resource(const char* data, u32 size, b8 auto_delete, resource_t* out_resource) {
    resource_type_t type = ((u16*)data)[1];
    // resource_t resource;
    switch (type) {
        default: 
            SERROR("Unable to load binary resource: Resource %d not implemented.", type);
            break;
        // case RESOURCE_TYPE_SHADER:
            // resource = pvt_shader_loader_load_binary_resource((void*)data, size, auto_delete);
            // return darray_resource_push(&loader_state->resources, resource);
    }
}
