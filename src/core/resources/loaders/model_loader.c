#include "Spark/resources/loaders/model_loader.h"
#include "Spark/containers/generic/darray_ints.h"
#include "Spark/core/sstring.h"
#include "Spark/defines.h"
#include "Spark/ecs/components/entity_child.h"
#include "Spark/ecs/components/entity_parent.h"
#include "Spark/ecs/ecs_world.h"
#include "Spark/ecs/entity.h"
#include "Spark/memory/block_allocator.h"
#include "Spark/memory/linear_allocator.h"
#include "Spark/platform/filesystem.h"
#include "Spark/renderer/material.h"
#include "Spark/renderer/model.h"
#include "Spark/renderer/renderer_frontend.h"
#include "Spark/renderer/shader.h"
#include "Spark/renderer/texture.h"
#include "Spark/resources/loaders/loader_utils.h"
#include "Spark/resources/loaders/material_loader.h"
#include "Spark/resources/resource_types.h"
#include "Spark/types/s3d.h"
#include "Spark/types/transforms.h"
#include <stb_image.h>

// Private types
darray_type(model_t*, model_ptr);
darray_impl(model_t*, model_ptr);

typedef struct model_loader_state {
    darray_u8_t file_buffer;
    block_allocator_t model_allocator;
    darray_model_ptr_t models;
} model_loader_state_t;

static model_loader_state_t* state = NULL;

// Private function prototypes
resource_t create_model_from_config(model_config_t* config);

void model_loader_initialzie(linear_allocator_t* allocator) {
    state = linear_allocator_allocate(allocator, sizeof(model_loader_state_t));
    block_allocator_create(1024, sizeof(model_t), &state->model_allocator);
    darray_model_ptr_create(1024, &state->models);
    darray_u8_create(8192, &state->file_buffer);
}

void model_loader_shutdown() {
    block_allocator_destroy(1024, sizeof(model_t), &state->model_allocator);
    darray_model_ptr_create(1024, &state->models);
    darray_u8_destroy(&state->file_buffer);
}

// resource_t* model_loader_load_resouce(const char* path, b8 auto_delete);
resource_t pvt_model_loader_load_text_resource(const char* text, u32 length, b8 auto_delete) {
    // Load line by line
    model_config_t config = {};

    const u32 max_args = 8;
    const u32 max_arg_length = 64;
    char args[max_args][max_arg_length];

    for (u32 offset = 0; offset < length;) {
        u32 arg_count = 0;
        loader_read_line(text, &offset, (char*)args, max_args, max_arg_length, &arg_count);

        if (string_equal(args[0], "path")) {
            config.path = args[1];
        }
    }

    return create_model_from_config(&config);
}

resource_t pvt_model_loader_load_binary_resource(void* binary_data, u32 size, b8 auto_delete) {
    SERROR("Unable to load binary model resource: Function Unimplemented");
    return (resource_t) {};
}

model_t* model_loader_get_model(u32 index);

// Private function impl
resource_t create_model_from_config(model_config_t* config) {
    file_handle_t file_handle;
    SASSERT(filesystem_open(config->path, FILE_MODE_READ, true, &file_handle), "Failed to open model at path '%s'", config->path);

    
    u64 file_size = 0;
    filesystem_get_file_size(&file_handle, &file_size);
    darray_u8_reserve(&state->file_buffer, file_size);

    u64 bytes_read = 0;
    filesystem_read_all_bytes(&file_handle, state->file_buffer.data, &bytes_read);
    filesystem_close(&file_handle);

    SASSERT(*(u32*)state->file_buffer.data == S3D_FILE_MAGIC, "Model '%s' is not an S3D mseh.", config->path);
    s3d_t* header = (s3d_t*)state->file_buffer.data;

    void* vertices = state->file_buffer.data + header->vertex_offset;
    void* indices = state->file_buffer.data + header->index_offset;

    // Load textures
    s3d_texture_t* s3d_textures = ((void*)header) + header->texture_offset;
    const u32 max_texture_count = 64;
    texture_t textures[max_texture_count];

    SASSERT(header->texture_count < max_texture_count, "S3d has too many textures, max of %d, got %d", max_texture_count, header->texture_count);

    for (u32 i = 0; i < header->texture_count; i++) {
        s32 width = 0;
        s32 height = 0;
        s32 components = 0;
        u8* pixels = stbi_load_from_memory(((void*)header) + s3d_textures[i].data_offset, s3d_textures[i].data_length, &width, &height, &components, 4);

        textures[i] = renderer_create_image_from_data((void*)pixels, width, height, 4, TEXTURE_FILTER_LINEAR);
        stbi_image_free(pixels);
    }

    // Load materials
    s3d_material_t* s3d_materials = ((void*)header) + header->material_offset;
    const u32 max_material_count = 64;
    material_t* materials[max_material_count];

    SASSERT(header->material_count < max_material_count, "S3d has too many materials, max of %d, got %d", max_material_count, header->material_count);

    for (u32 i = 0; i < header->material_count; i++) {
        // if (s3d_materials[i].texture_count <= 0) {
        //     continue;
        // }
        material_config_t material_config = {
            .resource_count = s3d_materials[i].texture_count,
        };
        string_copy(material_config.name, config->path);

        for (u32 t = 0; t < s3d_materials[i].texture_count; t++) {
            material_config.resources[t] = 
                (material_resource_t) {
                    .binding = t,
                    .set = 1,
                    .type = SHADER_RESOURCE_SAMPLER,
                    .value = &textures[s3d_materials[i].texture_indices[t]],
                };
        }

        materials[i] = material_loader_create_from_config(&material_config);
    }

    const u32 max_models = 1024;
    model_t* models[max_models];

    u32 resource_index = 0;
    for (u32 i = 0; i < header->object_count; i++) {
        s3d_object_t* object = (s3d_object_t*)(state->file_buffer.data + sizeof(s3d_t) + sizeof(s3d_object_t) * i);
        model_t* model = block_allocator_allocate(&state->model_allocator);
        models[i] = model;
        model->material_index = INVALID_ID_U16;
        model->child_count = object->child_count;

        if (object->parent_index != INVALID_ID) {
            const u32 parent = object->parent_index;
            if (models[parent]->children == NULL) {
                models[parent]->children = model;
            } else {
                model_t* child = models[parent]->children;
                while (child->next_child) {
                    child = child->next_child;
                }
                child->next_child = model;
            }
        } else {
            resource_index = state->models.count;
            darray_model_ptr_push(&state->models, model);
        }

        if (object->mesh_index != INVALID_ID_U16) {
            s3d_mesh_t* mesh = (s3d_mesh_t*)(state->file_buffer.data + header->mesh_offset + sizeof(s3d_mesh_t) * object->mesh_index);

            mesh_t out_mesh = renderer_create_mesh(vertices + mesh->vertex_offset, 
                    mesh->vertex_count, 
                    mesh->vertex_stride, 
                    indices + mesh->index_offset, 
                    mesh->index_count, 
                    mesh->index_stride);

            model->mesh = out_mesh;
            model->material_index = materials[mesh->material_index]->internal_index;
            model->bounds = mesh->bounds;
        } else {
            model->mesh.internal_index = INVALID_ID_U16;
        }

        model->translation = object->position;
        model->scale       = object->scale;
        model->rotation    = object->rotation;
    }


    resource_t resource = {
        .internal_index = resource_index,
        .type = RESOURCE_TYPE_MODEL,
    };
    return resource;
}

entity_t load_model_entity(ecs_world_t* world, model_t* model, entity_t parent, u32 material_count, material_t** materials) {
    entity_t e = entity_create(world);

    if (parent != INVALID_ID) {
        entity_add_child(world, parent, e);
    } 

    if (model->mesh.internal_index != INVALID_ID_U16) {
        ENTITY_SET_COMPONENT(world, e, mesh_t, model->mesh);

        if (model->material_index >= material_count) {
            ENTITY_SET_COMPONENT(world, e, material_t, *material_loader_get_material(model->material_index));
        } else {
            ENTITY_SET_COMPONENT(world, e, material_t, *materials[model->material_index]);
        }
        ENTITY_SET_COMPONENT(world, e, aabb_t, model->bounds);
    }


    ENTITY_SET_COMPONENT(world, e, translation_t, (translation_t) { model->translation });
    ENTITY_SET_COMPONENT(world, e, rotation_t,    (rotation_t)    { model->rotation});
    ENTITY_SET_COMPONENT(world, e, scale_t,       (scale_t)       { model->scale });
    ENTITY_SET_COMPONENT(world, e, dirty_transform_t, { true });
    ENTITY_ADD_COMPONENT(world, e, local_to_world_t);

    return e;
}

entity_t load_model_entity_recursive(ecs_world_t* world, model_t* node, entity_t parent, u32 material_count, material_t** materials) {
    entity_t e = load_model_entity(world, node, parent, material_count, materials);

    model_t* child = node->children;
    while (child) {
        load_model_entity_recursive(world, child, e, material_count, materials);
        child = child->next_child;
    }

    return e;
}

entity_t model_loader_instance_model(u32 index, u32 material_count, material_t** materials) {
    ecs_world_t* world = ecs_world_get();
    return load_model_entity_recursive(world, state->models.data[index], INVALID_ID, material_count, materials);
}
