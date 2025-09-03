#include "Spark/core/logging.h"
#include "Spark/defines.h"
#include "Spark/math/math_types.h"
#include <Spark/types/s3d.h>

#include <assimp/scene.h>
#include <assimp/mesh.h>
#include "Spark/math/vec3.h"
#include "assimp/cimport.h"
#include "assimp/color4.h"
#include "assimp/material.h"
#include "assimp/postprocess.h"
#include "assimp/quaternion.h"
#include "assimp/types.h"
#include "assimp/vector3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Private functions
void get_scene_index_vertex_size(const struct aiScene* scene, u32* out_vertex_size, u32* out_index_size);
vertex_attributes_t get_vertex_attributes_and_stride(const struct aiMesh* mesh, u16* out_stride);
s3d_mesh_t write_s3d_mesh(
        const struct aiMesh* mesh,
        vertex_3d_t* vertex_buffer,
        u64* vertex_offset,
        void* index_buffer,
        u64* index_offset);
u32 get_object_child_count(struct aiNode* object);
u32 get_scene_object_count(const struct aiScene* scene);
void get_scene_objects(s3d_object_t* objects, u32* index_ptr, struct aiNode* node, u32 parent_index);
static u32 written_object_count = 0;

int s3d_convert(const char* source_file, const char* out_file) {
    // Load scene
    const struct aiScene* scene = aiImportFile(source_file, aiProcess_Triangulate | aiProcess_ImproveCacheLocality | aiProcess_RemoveRedundantMaterials | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph | aiProcess_FindInstances | aiProcess_JoinIdenticalVertices | aiProcess_GenBoundingBoxes);
    if (!scene) {
        printf("Failed to load model file at path '%s'\n", source_file);
        return -1;
    }

    // Get the size of vertex and index output then allocate buffers
    u32 vertex_buffer_size = 0;
    u32 index_buffer_size = 0;
    get_scene_index_vertex_size(scene, &vertex_buffer_size, &index_buffer_size);

    void* vertex_buffer = malloc(vertex_buffer_size);
    void* index_buffer = malloc(index_buffer_size);
    memset(vertex_buffer, 0xFF, vertex_buffer_size);
    memset(index_buffer, 0xFF, index_buffer_size);

    // Write all objects
    u32 object_count = get_scene_object_count(scene);
    s3d_object_t* objects = malloc(sizeof(s3d_object_t) * object_count);
    u32 object_index_ptr = 0;
    get_scene_objects(objects, &object_index_ptr, scene->mRootNode, INVALID_ID);

    // Write meshes to buffers
    s3d_mesh_t* meshes = malloc(sizeof(s3d_mesh_t) * scene->mNumMeshes);

    u64 vertex_buffer_offset = 0;
    u64 index_buffer_offset = 0;
    for (int i = 0; i < scene->mNumMeshes; i++) {
        meshes[i] = write_s3d_mesh(scene->mMeshes[i], vertex_buffer, &vertex_buffer_offset, index_buffer, &index_buffer_offset);
    }
    
    // Write all textures
    u32* texture_offsets = malloc(sizeof(u32) * scene->mNumTextures);
    s3d_texture_t* textures = malloc(sizeof(u32) * scene->mNumTextures);
    for (u32 i = 0, offset = 0; i < scene->mNumTextures; i++) {
        u32 byte_count = 0;
        struct aiTexture* texture = scene->mTextures[i];
        if (texture->mHeight == 0) {
            byte_count = texture->mWidth;
        } else {
            byte_count = texture->mWidth * texture->mHeight;
        }

        texture_offsets[i] = offset;
        textures[i].data_length = byte_count;
        offset += byte_count;
    }

    // Materials
    s3d_material_t* materials = malloc(sizeof(s3d_material_t) * scene->mNumMaterials); 
    memset(materials, 0, sizeof(s3d_material_t) * scene->mNumMaterials); 
    for (u32 i = 0; i < scene->mNumMaterials; i++) {
        struct aiMaterial* material = scene->mMaterials[i];
        u32 texture_count = aiGetMaterialTextureCount(material, aiTextureType_DIFFUSE);
        materials[i].texture_count = texture_count;

        for (u32 t = 0; t < texture_count; t++) {
            const struct aiMaterialProperty* prop;
            aiReturn r = aiGetMaterialProperty(material, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, t), &prop);
            if (r == aiReturn_SUCCESS) {
                materials[i].texture_indices[t] = prop->mIndex;
            }
        }
    }

    s3d_t out_scene = {
        .file_magic = S3D_FILE_MAGIC,
        .object_count = object_count,
        .mesh_offset = sizeof(s3d_t) + sizeof(s3d_object_t) * object_count,
        .texture_count = scene->mNumTextures,
        .material_count = scene->mNumMaterials,
    };
    printf("Scene '%s' has %d / %d objects\n", source_file, out_scene.object_count, written_object_count);

    // Write the file
    FILE* file_ptr = fopen(out_file, "wb");
    if (!file_ptr) {
        printf("S3D converter failed to open file '%s'\n", out_file);
        return -1;
    }

    // Write temp header
    fwrite(&out_scene, sizeof(s3d_t), 1, file_ptr);                          // Header
    fwrite(objects, sizeof(s3d_object_t), out_scene.object_count, file_ptr); // Objects 

    out_scene.mesh_offset = ftell(file_ptr);
    fwrite(meshes, sizeof(s3d_mesh_t), scene->mNumMeshes, file_ptr);         // Meshes

    out_scene.vertex_offset = ftell(file_ptr);
    fwrite(vertex_buffer, vertex_buffer_size, 1, file_ptr);                  // vertices

    out_scene.index_offset = ftell(file_ptr);
    fwrite(index_buffer, index_buffer_size, 1, file_ptr);                    // indices

    // TODO: Materials
    out_scene.material_offset = ftell(file_ptr);
    fwrite(materials, sizeof(s3d_material_t), scene->mNumMaterials, file_ptr);

    // Textures
    out_scene.texture_offset = ftell(file_ptr);
    printf("texture offset: 0x%x\n", out_scene.texture_offset);

    for (u32 i = 0; i < scene->mNumTextures; i++) {
        s3d_texture_t temp_texture = {};
        fwrite(&temp_texture, sizeof(s3d_texture_t), 1, file_ptr);
    }

    for (u32 i = 0; i < scene->mNumTextures; i++) {
        textures[i].data_offset = ftell(file_ptr);
        fwrite(scene->mTextures[i]->pcData, textures[i].data_length, 1, file_ptr);
    }

    u32 current_offset = ftell(file_ptr);
    fseek(file_ptr, out_scene.texture_offset, SEEK_SET);
    for (u32 i = 0; i < scene->mNumTextures; i++) {
        printf("Writing textures %d with size 0x%x (offset 0x%x) to s3d ------------------------\n", i, textures->data_length, textures->data_offset);
        fwrite(&textures[i], sizeof(s3d_texture_t), 1, file_ptr);
    }
    fseek(file_ptr, current_offset, SEEK_SET);


    // Rewrite header
    fseek(file_ptr, 0, SEEK_SET);
    fwrite(&out_scene, sizeof(s3d_t), 1, file_ptr);                          // Header
    fclose(file_ptr);

    return 0;
}

// Private functions
/**
 * @brief Gets the total number of bytes that a scene will need to write the vertices and indices
 *
 * @param scene The ai scene
 * @param out_vertex_size output number of bytes used for all mesh vertices 
 * @param out_index_size output number of bytes used for all mesh indices
 */
void get_scene_index_vertex_size(const struct aiScene* scene, u32* out_vertex_size, u32* out_index_size) {
    *out_vertex_size = 0;
    *out_index_size = 0;

    for (int i = 0; i < scene->mNumMeshes; i++) {
        u32 vertex_stride = 0;
        const struct aiMesh* mesh = scene->mMeshes[i];
        if (mesh->mVertices) {
            vertex_stride += sizeof(vec3);
        }
        if (mesh->mTextureCoords[0]) {
            vertex_stride += sizeof(vec2);
        }
        if (mesh->mTextureCoords[1]) {
            vertex_stride += sizeof(vec2);
        }
        if (mesh->mTextureCoords[2]) {
            vertex_stride += sizeof(vec2);
        }
        if (mesh->mTextureCoords[3]) {
            vertex_stride += sizeof(vec2);
        }
        if (mesh->mNormals) {
            vertex_stride += sizeof(vec3);
        }
        if (mesh->mColors[0]) {
            vertex_stride += sizeof(vec4);
        }
        if (mesh->mColors[1]) {
            vertex_stride += sizeof(vec4);
        }
        if (mesh->mColors[2]) {
            vertex_stride += sizeof(vec4);
        }
        if (mesh->mColors[3]) {
            vertex_stride += sizeof(vec4);
        }

        *out_vertex_size += vertex_stride * mesh->mNumVertices;

        u32 index_stride = 2;
        if (mesh->mNumFaces * 3 > 0xFFFFu) {
            index_stride = 4;
        }
        index_stride = 4;
        *out_index_size += index_stride * mesh->mNumFaces * 3;

        if (*out_vertex_size % 0x10 != 0) {
            *out_vertex_size += 0x10 - (*out_vertex_size % 0x10);
        }
        if (*out_index_size % 0x10 != 0) {
            *out_index_size += 0x10 - (*out_index_size % 0x10);
        }
    }
}

s3d_mesh_t write_s3d_mesh(
        const struct aiMesh* mesh,
        vertex_3d_t* vertex_buffer,
        u64* vertex_offset,
        void* index_buffer,
        u64* index_offset) {
    // Initialize s3d_mesh
    s3d_mesh_t out_mesh = {
        .index_offset = *index_offset,
        .vertex_offset = *vertex_offset,
        .index_count = mesh->mNumFaces * 3,
        .vertex_count = mesh->mNumVertices,
    };

    // Calculate bounds
    vec3 bounds_min = { {mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z} };
    vec3 bounds_max = { {mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z} };
    vec3 bounds_center = vec3_mul_scalar(vec3_add(bounds_min, bounds_max), .5f);
    vec3 bounds_extents = vec3_sub(bounds_max, bounds_center);
    out_mesh.bounds = 
        (aabb_t) {
            .center = bounds_center,
            .extents = bounds_extents,
        };

    // Get vertex attributes and stride
    u16 vertex_stride = sizeof(vec3) * 2 + sizeof(vec2);
    if (mesh->mVertices) {
        for (int i = 0; i < mesh->mNumVertices; i++) {
            vertex_buffer[i].position = *(vec3*)&mesh->mVertices[i];
        }
    }
    if (mesh->mNormals) {
        for (int i = 0; i < mesh->mNumVertices; i++) {
            vertex_buffer[i].normal = *(vec3*)&mesh->mNormals[i];
        }
    }
    if (mesh->mTextureCoords[0]) {
        for (int i = 0; i < mesh->mNumVertices; i++) {
            vertex_buffer[i].uv = *(vec2*)&mesh->mTextureCoords[0][i];
        }
    }

    // Copy indices
    b8 is_short_index = false;

    for (int i = 0; i < mesh->mNumFaces; i++) {
        for (int f = 0; f < mesh->mFaces[i].mNumIndices; f++) {
            u32 index = mesh->mFaces[i].mIndices[f];
            if (is_short_index) {
                *(u16*)(index_buffer + *index_offset) = (u16)index;
                *index_offset += sizeof(u16);
            } else
            {
                *(u32*)(index_buffer + *index_offset) = index;
                *index_offset += sizeof(u32);
            }
        }
    }

    // Pad indices and vertices to nearest 0x10 alignment
    // if (*vertex_offset % 0x10 != 0) {
    //     *vertex_offset += 0x10 - (*vertex_offset % 0x10);
    // }
    // if (*index_offset % 0x10 != 0) {
    //     *index_offset += 0x10 - (*index_offset % 0x10);
    // }

    // Finish writing out_mesh
    out_mesh.vertex_stride = vertex_stride;
    out_mesh.index_stride = is_short_index ? sizeof(u16) : sizeof(u32);
    return out_mesh;
}

u32 get_object_child_count(struct aiNode* object) {
    u32 count = object->mNumChildren;
    for (u32 i = 0; i < object->mNumChildren; i++) {
        count += get_object_child_count(object->mChildren[i]);
    }

    return count;
}

u32 get_scene_object_count(const struct aiScene* scene) {
    return get_object_child_count(scene->mRootNode) + 1;
}

void get_scene_objects(s3d_object_t* objects, u32* index_ptr, struct aiNode* node, u32 parent_index) {
    // TODO: Multiple meshes per object
    u32 index = *index_ptr;
    objects[index].mesh_index = INVALID_ID_U16;
    objects[index].child_count= node->mNumChildren;
    if (node->mNumMeshes > 0) {
        objects[index].mesh_index = node->mMeshes[0];
    } 
    objects[index].parent_index = parent_index;

    struct aiVector3D position, scale;
    struct aiQuaternion rotation;
    aiDecomposeMatrix(&node->mTransformation, &scale, &rotation, &position);
    objects[index].position = 
        (vec3) {
            .x = position.x,
            .y = position.y,
            .z = position.z,
        };
    objects[index].scale = 
        (vec3) {
            .x = scale.x,
            .y = scale.y,
            .z = scale.z,
        };
    objects[index].rotation = 
        (quat) {
            .x = rotation.x,
            .y = rotation.y,
            .z = rotation.z,
            .w = rotation.w,
        };
    written_object_count++;
    *index_ptr = *index_ptr + 1;

    for (u32 i = 0; i < node->mNumChildren; i++) {
        get_scene_objects(objects, index_ptr, node->mChildren[i], index);
    }
}

