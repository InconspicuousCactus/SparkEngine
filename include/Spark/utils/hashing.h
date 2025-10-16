#pragma once
#include "Spark/core/logging.h"
#include "Spark/core/sstring.h"
#include "Spark/defines.h"
#include "Spark/math/math_types.h"

typedef u64 hash_t;

#define X_PRIME 501125321
#define Y_PRIME 1136930381
#define Z_PRIME 1720413743
#define W_PRIME 1066037191

SINLINE hash_t string_hash(const char* string) {
    u64 hash = 5381;
    char c = *string;

    while (c != 0) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        c = *string++;
    }

    return hash;
}

SINLINE b8 u64_compare(u64 a, u64 b) {
    return a == b;
}
SINLINE hash_t hash_passthrough(u64 key) {
    return key;
}

// Stolen from stackoverflow. Performance may not be ideal
SINLINE hash_t hash_u64(u64 key) {
    return key;
}

SINLINE b8 vec2i_compare(vec2i a, vec2i b) {
    return a.x == b.x && a.y == b.y;
}
SINLINE hash_t hash_vec2i(vec2i key) {
    return (key.x + X_PRIME) ^
           (key.y + Y_PRIME);
}

SINLINE b8 vec3i_compare(vec3i a, vec3i b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
SINLINE hash_t hash_vec3i(vec3i key) {
    return (key.x + X_PRIME) ^
           (key.y + Y_PRIME) ^
           (key.z + Z_PRIME);
}

SINLINE b8 vec4i_compare(vec4i a, vec4i b) {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
SINLINE hash_t hash_vec4i(vec4i key) {
    return (key.x + X_PRIME) ^
           (key.y + Y_PRIME) ^
           (key.z + Z_PRIME) ^
           (key.w + W_PRIME);
}

SINLINE b8 string_compare(const char* a, const char* b) {
    SDEBUG("String compare: %p - %p", a, b);
    SDEBUG("String compare: %s - %s", a, b);
    return string_equal(a, b);
    if (!a || !b) {
        return false;
    }
    char ca = a[0];
    char cb = b[0];
    while (ca != 0 && cb != 0) {
        if (ca != cb) {
            return false;
        }
        a++;
        b++;
        ca = *a;
        cb = *b;
    }

    return true;
}
