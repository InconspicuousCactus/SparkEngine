#pragma once

#include "Spark/ecs/ecs.h"
#include "Spark/ecs/entity.h"
#include "Spark/math/math_types.h"

typedef struct text {
    const char* value;
} text_t;
extern ECS_COMPONENT_DECLARE(text_t);

entity_t text_create(ecs_world_t* world, const char* text, vec2 anchor, f32 point_size);
