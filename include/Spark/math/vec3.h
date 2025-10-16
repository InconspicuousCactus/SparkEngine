#pragma once 
#include "Spark/math/math_types.h"
#include "Spark/math/smath.h"

// ==================================
// Vec3
// ==================================
SINLINE vec3 vec3_create(f32 x, f32 y, f32 z) {
    vec3 out = {
        .x = x,
        .y = y,
        .z = z,
    };
    return out;
}

SINLINE vec4 vec3_to_vec4(vec3 vector, f32 w) {
    return (vec4){
        .x = vector.x,
        .y = vector.y,
        .z = vector.z,
        .w = w,
    };
}

SINLINE vec3 vec3_zero() {
    vec3 out = {
    };
    return out;
}

SINLINE vec3 vec3_one() {
    vec3 out = {
        .x = 1.0f,
        .y = 1.0f,
        .z = 1.0f,
    };
    return out;
}

SINLINE vec3 vec3_up() {
    vec3 out = {
        .x = 0.0f,
        .y = 1.0f,
        .z = 0.0f,
    };
    return out;
}

SINLINE vec3 vec3_down() {
    vec3 out = {
        .x = 0.0f,
        .y = -1.0f,
        .z = 0.0f,
    };
    return out;
}

SINLINE vec3 vec3_right() {
    vec3 out = {
        .x = 1.0f,
        .y = 0.0f,
        .z = 0.0f,
    };
    return out;
}

SINLINE vec3 vec3_left() {
    vec3 out = {
        .x = -1.0f,
        .y = 0.0f,
        .z = 0.0f,
    };
    return out;
}

SINLINE vec3 vec3_forward() {
    vec3 out = {
        .x = 0.0f,
        .y = 0.0f,
        .z = -1.0f,
    };
    return out;
}

SINLINE vec3 vec3_back() {
    vec3 out = {
        .x = 0.0f,
        .y = 0.0f,
        .z = 1.0f,
    };
    return out;
}

SINLINE vec3 vec3_add(vec3 a, vec3 b) { 
    return (vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

SINLINE vec3 vec3_sub(vec3 a, vec3 b) { 
    return (vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

SINLINE vec3 vec3_mul(vec3 a, vec3 b) { 
    return (vec3){a.x * b.x, a.y * b.y, a.z * b.z};
}

SINLINE vec3 vec3_div(vec3 a, vec3 b) { 
    return (vec3){a.x / b.x, a.y / b.y, a.z / b.z};
}

SINLINE vec3 vec3_mul_scalar(vec3 vector, f32 scalar) { 
    return (vec3){vector.x * scalar, vector.y * scalar, vector.z * scalar};
}

SINLINE f32 vec3_length_squared(vec3 vector) {
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
}

SINLINE f32 vec3_length(vec3 vector) {
    return ssqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
}

SINLINE void vec3_normalize(vec3* vector) {
    const f32 length = vec3_length(*vector);
    vector->x /= length;
    vector->y /= length;
    vector->z /= length;
}

SINLINE vec3 vec3_normalized(vec3 vector) {
    const f32 length = vec3_length(vector);
    vector.x /= length;
    vector.y /= length;
    vector.z /= length;
    return vector;
}

SINLINE b8 vec3_compare(vec3 a, vec3 b, f32 tolerance) {
    if (sabs(a.x - b.x) > tolerance) { 
        return false;
    }
    if (sabs(a.y - b.y) > tolerance) {
        return false;
    }
    if (sabs(a.z - b.z) > tolerance) {
        return false;
    }
    return true;
}

SINLINE f32 vec3_distance_sqr(vec3 a, vec3 b) {
    vec3 d = (vec3){
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
    };
    return vec3_length_squared(d);
}

SINLINE f32 vec3_distance(vec3 a, vec3 b) {
    vec3 d = (vec3){
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
    };
    return vec3_length(d);
}

SINLINE f32 vec3_dot(vec3 a, vec3 b) {
    return
        a.x * b.x +
        a.y * b.y +
        a.z * b.z;
}

SINLINE vec3 vec3_cross(vec3 a, vec3 b) {
    return (vec3){
        .x = a.y * b.z - a.z * b.y,
        .y = a.z * b.x - a.x * b.z,
        .z = a.x * b.y - a.y * b.x,
    };
}
