#pragma once

#include "Spark/ecs/ecs.h"
#include "Spark/math/math_types.h"
typedef struct anchor_2d {
    vec2 pos;
} anchor_2d_t;

extern ECS_COMPONENT_DECLARE(anchor_2d_t);
