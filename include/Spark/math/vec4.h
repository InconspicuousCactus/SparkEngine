#pragma once 
#include "Spark/math/math_types.h"
#include "Spark/math/smath.h"

// ==================================
// Vec4
// ==================================
SINLINE vec4 vec4_create(f32 x, f32 y, f32 z, f32 w) {
    vec4 out = {
        .x = x,
        .y = y,
        .z = z,
        .w = w,
    };
    return out;
}

SINLINE vec3 vec4_to_vec3(vec4 vector) {
    return (vec3){
        .x = vector.x,
        .y = vector.y,
        .z = vector.z,
    };
}

SINLINE vec4 vec4_from_vec3(vec3 vector, f32 w) {
    return (vec4){
        .x = vector.x,
        .y = vector.y,
        .z = vector.z,
        .w = w,
    };
}

SINLINE vec4 vec4_zero() {
    vec4 out = { };
    return out;
}

SINLINE vec4 vec4_one() {
    vec4 out = {
        .x = 1.0f,
        .y = 1.0f,
        .z = 1.0f,
        .w = 1.0f,
    };
    return out;
}

SINLINE vec4 vec4_add(vec4 a, vec4 b) { 
    vec4 result = {};
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}

SINLINE vec4 vec4_sub(vec4 a, vec4 b) { 
    vec4 result = {};
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return result;
}

SINLINE vec4 vec4_mul(vec4 a, vec4 b) { 
    vec4 result = {};
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    result.w = a.w * b.w;
    return result;
}

SINLINE vec4 vec4_div(vec4 a, vec4 b) { 
    vec4 result = {};
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    result.w = a.w / b.w;
    return result;
}

SINLINE f32 vec4_length_squared(vec4 vector) {
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w * vector.w;
}

SINLINE f32 vec4_length(vec4 vector) {
    return ssqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
}

SINLINE void vec4_normalize(vec4* vector) {
    const f32 length = vec4_length(*vector);
    vector->x /= length;
    vector->y /= length;
    vector->z /= length;
    vector->w /= length;
}

SINLINE vec4 vec4_normalized(vec4 vector) {
    const f32 length = vec4_length(vector);
    vector.x /= length;
    vector.y /= length;
    vector.z /= length;
    vector.w /= length;
    return vector;
}

SINLINE b8 vec4_compare(vec4 a, vec4 b, f32 tolerance) {
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

SINLINE f32 vec4_distance(vec4 a, vec4 b) {
    vec4 d = (vec4){
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
    };
    return vec4_length(d);
}

SINLINE f32 vec4_dot(vec4 a, vec4 b) {
    return
        a.x * b.x +
        a.y * b.y +
        a.z * b.z;
}

SINLINE f32 vec4_dot_f32(
        f32 a0, f32 a1, f32 a2, f32 a3,
        f32 b0, f32 b1, f32 b2, f32 b3) {
    return
        a0 * b0 + 
        a1 * b1 + 
        a2 * b2 + 
        a3 * b3 ;
}
