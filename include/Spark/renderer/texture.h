#pragma once

#include "Spark/containers/darray.h"
typedef enum texture_filter : u8 {
    TEXTURE_FILTER_LINEAR,
    TEXTURE_FILTER_NEAREST,
} texture_filter_t;

typedef struct texture {
    u32 internal_offset;
} texture_t;

darray_header(texture_t, texture);
