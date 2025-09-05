#pragma once

#include "Spark/math/math_types.h"

typedef struct directional_light {
    vec3 direction;
    vec3 color;
    float power;
} directional_light_t;

typedef struct point_light {
    vec3 position;
    vec3 color;
    float power;
} point_light_t;
