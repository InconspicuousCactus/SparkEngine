#pragma once 
#include "Spark/math/math_types.h"
#include "Spark/math/smath.h"

// ==================================
// Vec2
// ==================================
SINLINE vec2 vec2_create(f32 x, f32 y) {
    vec2 out = {
        .x = x,
        .y = y,
    };
    return out;
}

SINLINE vec2 vec2_zero() {
    vec2 out = { };
    return out;
}

SINLINE vec2 vec2_one() {
    vec2 out = {
        .x = 1.0f,
        .y = 1.0f,
    };
    return out;
}

SINLINE vec2 vec2_up() {
    vec2 out = {
        .x = 0.0f,
        .y = 1.0f,
    };
    return out;
}

SINLINE vec2 vec2_down() {
    vec2 out = {
        .x = 0.0f,
        .y = -1.0f,
    };
    return out;
}

SINLINE vec2 vec2_right() {
    vec2 out = {
        .x = 1.0f,
        .y = 0.0f,
    };
    return out;
}

SINLINE vec2 vec2_left() {
    vec2 out = {
        .x = -1.0f,
        .y = 0.0f,
    };
    return out;
}

SINLINE vec2 vec2_add(vec2 a, vec2 b) { 
    return (vec2){a.x + b.x, a.y + b.y};
}

SINLINE vec2 vec2_sub(vec2 a, vec2 b) { 
    return (vec2){a.x - b.x, a.y - b.y};
}

SINLINE vec2 vec2_mul(vec2 a, vec2 b) { 
    return (vec2){a.x * b.x, a.y * b.y};
}

SINLINE vec2 vec2_div(vec2 a, vec2 b) { 
    return (vec2){a.x / b.x, a.y / b.y};
}

SINLINE f32 vec2_length_squared(vec2 vector) {
    return vector.x * vector.x + vector.y * vector.y;
}

SINLINE f32 vec2_length(vec2 vector) {
    return ssqrt(vector.x * vector.x + vector.y * vector.y);
}

SINLINE void vec2_normalize(vec2* vector) {
    const f32 length = vec2_length(*vector);
    vector->x /= length;
    vector->y /= length;
}

SINLINE vec2 vec2_normalized(vec2 vector) {
    const f32 length = vec2_length(vector);
    vector.x /= length;
    vector.y /= length;
    return vector;
}

SINLINE b8 vec2_compare(vec2 a, vec2 b, f32 tolerance) {
    if (sabs(a.x - b.x) > tolerance) { 
        return false;
    }
    if (sabs(a.y - b.y) > tolerance) {
        return false;
    }
    return true;
}

SINLINE f32 vec2_distance(vec2 a, vec2 b) {
    vec2 d = (vec2){
        .x = a.x - b.x,
            .y = a.y - b.y,
    };
    return vec2_length(d);
}

