#pragma once

#include "Spark/math/mat4.h"
#include "Spark/renderer/lights.h"
typedef enum builtin_renderpass {
    BUILTIN_RENDERPASS_SKYBOX,
    BUILTIN_RENDERPASS_WORLD,
    BUILTIN_RENDERPASS_UI,
    BUILTIN_RENDERPASS_ENUM_MAX,
} builtin_renderpass_t;

typedef struct renderpass_skybox_data {
} renderpass_skybox_data_t;

typedef struct renderpass_world_data {
    mat4                view_matrix;
    directional_light_t directional_lights[4];
    point_light_t       point_lights[4];
} renderpass_world_data_t;

typedef struct renderpass_ui_data {
} renderpass_ui_data_t;
