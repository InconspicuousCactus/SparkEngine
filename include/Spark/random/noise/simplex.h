#pragma once
#include "Spark/math/math_types.h"

float simplex_2d                (s32 seed, vec2 pos);
s32 simplex_2d_int              (s32 seed, vec2i pos);
void simplex_2d_int_simd        (s32 seed, vec2i pos, vec2i size, s32 scale, s32* out_noise);
void simplex_2d_int_simd_octaves(s32 seed, vec2i pos, vec2i size, s32 scale, u32 octaves, s32 persistance, s32 lacunarity, s32* out_noise);

f32 simplex_3d(vec3 pos);
s32 simplex_3d_int(vec3i pos);
