#pragma once

#include "Spark/ecs/ecs.h"
#include "Spark/ecs/entity.h"
#include "Spark/math/math_types.h"

typedef struct text {
    const char* value;
    f32 point_size;
} text_t;
extern ECS_COMPONENT_DECLARE(text_t);

entity_t text_create(ecs_world_t* world, text_t text, vec2 anchor);
void text_update(ecs_world_t* world, entity_t entity, text_t new_text);
