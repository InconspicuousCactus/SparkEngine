#include "Spark/resources/loaders/shader_loader.h"
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
}
void shader_loader_shutdown() {
    darray_shader_destroy(&loader_state->shaders);
}

resource_t pvt_shader_loader_load_text_resource(const char* text, u32 length, b8 auto_delete) {
    // Load line by line
    const u32 max_args = 8;
    const u32 max_arg_length = 64;
    char arg_buffer[max_args][max_arg_length];

    shader_config_t config = {};

    for (u32 offset = 0; offset < length;) {
        u32 arg_count = 0;
        loader_read_line(text, &offset, (char*)arg_buffer, max_args, max_arg_length, &arg_count);

        // Skip empty lines
        if (arg_buffer[0][0] == '0') {
            continue;
        }

        // Set config based on data
        if (string_equal(arg_buffer[0], "name")) {
          string_copy(config.name, arg_buffer[1]);
        } else if (string_equal(arg_buffer[0], "stage")) {
            SASSERT(arg_count > 1, "Cannot create shader from text resource without any stages.");
            for (u32 i = 1; i < arg_count; i++) {
                const char* stage = arg_buffer[i];
                if (string_equal(stage, "vert") ||string_equal(stage, "vertex")) {
                    config.stages |= SHADER_STAGE_VERTEX;
                } else if (string_equal(stage, "frag") ||string_equal(stage, "fragment")) {
                    config.stages |= SHADER_STAGE_FRAGMENT;
                } else {
                    SWARN("Unrecognized shader stage in shader resource: '%s'", stage); 
                }
            }
        } else if (string_equal(arg_buffer[0], "vertex_attributes")) {
            config.attribute_count = 0;
            for (u32 i = 1; i < arg_count; i++) {
                const char* attribute = arg_buffer[i];
                if (string_equal(attribute, "float")) {
                    config.attributes[config.attribute_count++] = VERTEX_ATTRIBUTE_FLOAT;
                } else if (string_equal(attribute, "vec2")) {
                    config.attributes[config.attribute_count++] = VERTEX_ATTRIBUTE_VEC2;
                } else if (string_equal(attribute, "vec3")) {
                    config.attributes[config.attribute_count++] = VERTEX_ATTRIBUTE_VEC3;
                } else if (string_equal(attribute, "vec4")) {
                    config.attributes[config.attribute_count++] = VERTEX_ATTRIBUTE_VEC4;
                } else if (string_equal(attribute, "int")) {
                    config.attributes[config.attribute_count++] = VERTEX_ATTRIBUTE_INT;
                } else if (string_equal(attribute, "vec2i")) {
                    config.attributes[config.attribute_count++] = VERTEX_ATTRIBUTE_IVEC2;
                } else if (string_equal(attribute, "vec3i")) {
                    config.attributes[config.attribute_count++] = VERTEX_ATTRIBUTE_IVEC3;
                } else if (string_equal(attribute, "vec4i")) {
                    config.attributes[config.attribute_count++] = VERTEX_ATTRIBUTE_IVEC4;
                } else {
                    SWARN("Shader has unknown attribute '%s'", attribute);
                }
            }
        } else if (string_equal(arg_buffer[0], "resource")) {
            u32 resource_index = config.resource_count;
            SASSERT(resource_index < SHADER_MAX_RESOURCES, "Cannot add more than %d resources from shader resource.", SHADER_MAX_RESOURCES);
            config.resource_count++;

            // Get type
            if (string_equal(arg_buffer[1], "uniform_buffer")) {
                config.layout[resource_index].type = SHADER_RESOURCE_UNIFORM_BUFFER;
            } else if (string_equal(arg_buffer[1], "sampler")) {
                config.layout[resource_index].type = SHADER_RESOURCE_SAMPLER;
            } else if (string_equal(arg_buffer[1], "buffer")) {
                config.layout[resource_index].type = SHADER_RESOURCE_SOTRAGE_BUFFER;
            } else {
                SWARN("Shader '%s' has unknown resource type '%s'", config.name, arg_buffer[1]);
            }

            // Get stage
            if (string_equal(arg_buffer[2], "vert") || string_equal(arg_buffer[2], "vertex")) {
                config.layout[resource_index].stage |= SHADER_STAGE_VERTEX;
            } else if (string_equal(arg_buffer[2], "frag") || string_equal(arg_buffer[2], "fragment")) {
                config.layout[resource_index].stage |= SHADER_STAGE_FRAGMENT;
            } else {
                SCRITICAL("Failed to get shader stage for text resource. Got '%s', possible options are 'vert', 'vertex', 'frag', and 'fragment'", arg_buffer[2]);
            }

            u32 binding = 0;
            u32 set = 0;

            string_to_u32(arg_buffer[3], &binding);
            string_to_u32(arg_buffer[4], &set);
            
            config.layout[resource_index].binding = binding;
            config.layout[resource_index].set = set;
        } else if (string_equal(arg_buffer[0], "type")) {
            if (string_equal(arg_buffer[1], "3d")) {
                config.type = BUILTIN_RENDERPASS_WORLD;
            } else if (string_equali(arg_buffer[1], "ui")) {
                config.type = BUILTIN_RENDERPASS_UI;
            } else {
                SWARN("Unknown shader type '%s': Defaulting to 3d.", arg_buffer[1]);
                config.type = BUILTIN_RENDERPASS_WORLD;
            }
        } else {
            SWARN("Failed to get shader resource key: '%s'", arg_buffer[0]);
        }
    }

    // Create shader from config
    shader_t shader = renderer_create_shader(&config);
#ifdef SPARK_DEBUG
    string_copy(shader.name, config.name);
#endif
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
