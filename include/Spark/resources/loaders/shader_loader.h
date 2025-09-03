#pragma once

#include "Spark/memory/linear_allocator.h"
#include "Spark/renderer/shader.h"
#include "Spark/resources/resource_types.h"

void shader_loader_initialzie(linear_allocator_t* allocator);
void shader_loader_shutdown();

// resource_t* shader_loader_load_resouce(const char* path, b8 auto_delete);
resource_t pvt_shader_loader_load_text_resource(const char* text, u32 length, b8 auto_delete);
resource_t pvt_shader_loader_load_binary_resource(void* binary_data, u32 size, b8 auto_delete);
shader_t* shader_loader_get_shader(u32 index);
