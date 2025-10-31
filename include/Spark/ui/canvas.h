#pragma once

#include "Spark/defines.h"

typedef enum canvas_mode : u8 {
    CANVAS_MODE_RELATIVE_SIZE,
    CANVAS_MODE_FIXED_SIZE,
} canvas_mode_t;

typedef struct canvas {
    f32 aspect_ratio;
    canvas_mode_t mode;
} canvas_t;
