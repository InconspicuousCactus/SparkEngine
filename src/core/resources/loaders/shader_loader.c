#include "Spark/resources/loaders/shader_loader.h"
#include "Spark/core/logging.h"
#include "Spark/core/sstring.h"
#include "Spark/memory/linear_allocator.h"
#include "Spark/platform/filesystem.h"
#include "Spark/renderer/renderer_frontend.h"
#include "Spark/renderer/renderpasses.h"
#include "Spark/renderer/shader.h"
#include "Spark/resources/loaders/loader_utils.h"
#include "Spark/resources/resource_types.h"
#include "Spark/types/s3d.h"

// =================================
// Private Data Types
// =================================
darray_impl(shader_t, shader);

typedef struct shader_loader_state {
    darray_shader_t shaders;
    darray_u8_t vertex_spv_buffer;
    darray_u8_t fragment_spv_buffer;
} shader_loader_state_t;

static shader_loader_state_t* loader_state;

// =================================
// Private Functions
// =================================
resource_t create_resource_from_shader(shader_t shader, b8 auto_delete);

// =================================
// Function Implementations
// =================================
void shader_loader_initialzie(linear_allocator_t* allocator) {
    loader_state = linear_allocator_allocate(allocator, sizeof(shader_loader_state_t));
    darray_shader_create(256, &loader_state->shaders);
    darray_u8_create(8192, &loader_state->vertex_spv_buffer);
    darray_u8_create(8192, &loader_state->fragment_spv_buffer);
}
void shader_loader_shutdown() {
    darray_shader_destroy(&loader_state->shaders);
    darray_u8_destroy(&loader_state->vertex_spv_buffer);
    darray_u8_destroy(&loader_state->fragment_spv_buffer);
}

resource_t pvt_shader_loader_load_text_resource(const char* text, u32 length, b8 auto_delete) {
    SCRITICAL("Text shader resources are depricated. Cannot load shader '%s'", text);


    shader_config_t config = {};
    // Create shader from config
    shader_t shader = renderer_create_shader(&config);
    return create_resource_from_shader(shader, auto_delete);
}

resource_t pvt_shader_loader_load_binary_resource(void* binary_data, u32 size, b8 auto_delete) {
    binary_shader_resource_t* resource = binary_data;

    shader_config_t config = resource->config;
    config.vertex_spv += (size_t)binary_data;
    config.fragment_spv += (size_t)binary_data;

    shader_t shader = renderer_create_shader(&config);
    return create_resource_from_shader(shader, auto_delete);
}

resource_t create_resource_from_shader(shader_t shader, b8 auto_delete) {
    u32 shader_index = loader_state->shaders.count;
    darray_shader_push(&loader_state->shaders, shader);

    resource_t resource = {
        .internal_index = shader_index,
        .auto_delete = auto_delete,
        .generation = 0,
        .ref_count = 1,
        .type = RESOURCE_TYPE_SHADER,
    };

    return resource;
}

shader_t* shader_loader_get_shader(u32 index) {
    SASSERT(index < loader_state->shaders.count && index >= 0, "Cannot load shader with out of bounds index. Index: %d, Shader Count: %d", index, loader_state->shaders.count);
    return &loader_state->shaders.data[index];
}
