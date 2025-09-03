#pragma once

#include "Spark/memory/linear_allocator.h"
#include "Spark/renderer/material.h"
#include "Spark/renderer/model.h"
#include "Spark/resources/resource_types.h"

void model_loader_initialzie(linear_allocator_t* allocator);
void model_loader_shutdown();

// resource_t* model_loader_load_resouce(const char* path, b8 auto_delete);
resource_t pvt_model_loader_load_text_resource(const char* text, u32 length, b8 auto_delete);
resource_t pvt_model_loader_load_binary_resource(void* binary_data, u32 size, b8 auto_delete);
model_t* model_loader_get_model(u32 index);
entity_t model_loader_instance_model(u32 index, u32 material_count, material_t** materials);
