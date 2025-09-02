#include "Spark/math/smath.h"
#include "Spark/random/noise/simplex.h"
#include <immintrin.h>
#include <nmmintrin.h>
#pragma GCC target("avx2")

// Constant Numbers
#define F2 (0.5 * (S_SQRT_THREE - 1.0))
#define F3 (1.0 / 3.0)
#define G2 ((3.0 - S_SQRT_THREE) / 6.0)
#define G3 (1.0 / 6.0)
#define INT_ONE_BIT_COUNT 12
#define INT_ONE ((1 << INT_ONE_BIT_COUNT))

#define X_PRIME 501125321
#define Y_PRIME 1136930381
#define Z_PRIME 1720413743
#define W_PRIME 1066037191

// ===============================
// Private functions
// ===============================
// Math
#define fast_floor_int(x) (x & ~(INT_ONE - 1))
s32 fast_floor(const f32 x);

// Scalar
SINLINE s32 hash_primes(u32 seed, s32 a, s32 b);
SINLINE f32 calculate_t(f32 x, f32 y);
SINLINE f32 get_gradient_dot_fancy(s32 hash, f32 fX, f32 fY);
SINLINE s32 calculate_t_int(s32 x, s32 y);
SINLINE s32 get_gradient_dot_fancy_int(s32 hash, s32 fX, s32 fY);

// Simd
SINLINE __m256i simd_get_gradient_dot_fancy(__m256i hash, __m256i fX, __m256i fY );
SINLINE __m256i simd_hash_primes(u64 seed, __m256i a, __m256i b);
SINLINE __m256i simd_calculate_t(__m256i x, __m256i y);

SINLINE void simplex_2d_int_simd_row(const u32 seed, __m256i x, __m256i y, __m256i* output);

float simplex_2d(const s32 seed, vec2 pos) {
    const f32 f = (pos.x + pos.y) * F2;
    f32 x0 = fast_floor(pos.x + f);
    f32 y0 = fast_floor(pos.y + f);

    const f32 g = G2 * (x0 + y0);
    const s32 i = (s32)x0 * X_PRIME;
    const s32 j = (s32)y0 * Y_PRIME;

    x0 = pos.x - (x0 - g);
    y0 = pos.y - (y0 - g);

    const b8 i1 = x0 > y0;
    f32 x1 = i1 ? x0 - 1 : x0;
    f32 y1 = i1 ? y0     : y0 - 1;

    x1 += G2;
    y1 += G2;

    f32 x2 = x0 + (G2 * 2 - 1);
    f32 y2 = y0 + (G2 * 2 - 1);

    f32 t0 = calculate_t(x0, y0);
    f32 t1 = calculate_t(x1, y1);
    f32 t2 = calculate_t(x2, y2);

    f32 n0 = get_gradient_dot_fancy(
                hash_primes(seed, i, j),
                x0, y0);

    f32 n1 = get_gradient_dot_fancy(
                hash_primes(seed, 
                    i1 ? i + X_PRIME : i, 
                    i1 ? j           : j + Y_PRIME), 
                x1, y1);

    f32 n2 = get_gradient_dot_fancy(
                hash_primes(seed, 
                    i + X_PRIME,
                    j + Y_PRIME), 
                x2, y2);

    f32 noise = (n0 * t0 + n1 * t1 + n2 * t2) * 38.283687591552734375;
    return noise;

}
/**
 * @brief Simplex noise using integers instead of floats. 65535 maps to 1.0
 *
 * @param pos Position in integer space
 * @return Simplex noise value
 */
s32 simplex_2d_int(const s32 seed, vec2i pos) {
    const s32 f = ((pos.x + pos.y) * (s32)(F2 * INT_ONE)) >> INT_ONE_BIT_COUNT;
    s32 x0 = fast_floor_int((pos.x + f));
    s32 y0 = fast_floor_int((pos.y + f));

    const s32 g = ((s32)(INT_ONE * G2) * (x0 + y0)) >> INT_ONE_BIT_COUNT;
    const s32 i = (x0 >> INT_ONE_BIT_COUNT) * X_PRIME;
    const s32 j = (y0 >> INT_ONE_BIT_COUNT) * Y_PRIME;

    x0 = pos.x - (x0 - g);
    y0 = pos.y - (y0 - g);

    const b8 i1 = x0 > y0;
    s32 x1 = i1 ? x0 - INT_ONE : x0;
    s32 y1 = i1 ? y0           : y0 - INT_ONE;

    x1 += (s32)(INT_ONE * G2);
    y1 += (s32)(INT_ONE * G2);

    s32 x2 = x0 + (s32)(INT_ONE * (G2 * 2 - 1));
    s32 y2 = y0 + (s32)(INT_ONE * (G2 * 2 - 1));

    s32 t0 = calculate_t_int(x0, y0);
    s32 t1 = calculate_t_int(x1, y1);
    s32 t2 = calculate_t_int(x2, y2);

    s32 n0 = get_gradient_dot_fancy_int(
                hash_primes(seed, i, j),
                x0, y0);

    s32 n1 = get_gradient_dot_fancy_int(
                hash_primes(seed, 
                    i1 ? i + X_PRIME : i, 
                    i1 ? j           : j + Y_PRIME), 
                x1, y1);

    s32 n2 = get_gradient_dot_fancy_int(
                hash_primes(seed, 
                    i + X_PRIME,
                    j + Y_PRIME), 
                x2, y2);

    s32 noise = ((n0 * t0) >> INT_ONE_BIT_COUNT)  + 
                ((n1 * t1) >> INT_ONE_BIT_COUNT)  + 
                ((n2 * t2) >> INT_ONE_BIT_COUNT);

    noise *= ((s32)38.283687591552734375 * INT_ONE);
    noise >>= INT_ONE_BIT_COUNT;
    return noise;
}

void simplex_2d_int_simd_octaves(s32 seed, vec2i pos, vec2i size, s32 scale, u32 octaves, s32 persistance, s32 lacunarity, s32* out_noise) {
    // Constants
    const __m256i scale_simd = _mm256_set1_epi32(scale);
    const __m256i lacunarity_simd = _mm256_set1_epi32(lacunarity);
    const __m256i persistance_simd = _mm256_set1_epi32(persistance);
    const s32 add_vector[8] __attribute__((aligned (32))) = { 0, 1, 2, 3, 4, 5, 6, 7 };
    const u32 simd_size = sizeof(__m256i) / 4;

    __m256i add = _mm256_stream_load_si256((const void*)add_vector);
            add = _mm256_mullo_epi32(add, scale_simd);

    for (u32 _y = 0, index = 0; _y < size.y; _y++) {
        for (u32 _x = 0; _x < size.x; _x += simd_size, index++) {
            __m256i y = _mm256_set1_epi32((pos.y + _y) * scale);
            __m256i x = _mm256_add_epi32(add, _mm256_set1_epi32((pos.x + _x) * scale));

            __m256i noise = _mm256_set1_epi32(0);
            __m256i per_simd = _mm256_set1_epi32(INT_ONE);
            for (u32 i = 0; i < octaves; i++) {
                __m256i tmp;
                simplex_2d_int_simd_row(seed, x, y, &tmp);

                // tmp *= per
                tmp = _mm256_mullo_epi32(tmp, per_simd);
                tmp = _mm256_srai_epi32(tmp, INT_ONE_BIT_COUNT);

                // Noise += tmp
                noise  = _mm256_add_epi32(tmp, noise);

                // (x,y) *= lac
                x = _mm256_mullo_epi32(x, lacunarity_simd);
                x = _mm256_srai_epi32(x, INT_ONE_BIT_COUNT);
                y = _mm256_mullo_epi32(y, lacunarity_simd);
                y = _mm256_srai_epi32(y, INT_ONE_BIT_COUNT);

                // per *= persistance
                per_simd = _mm256_mullo_epi32(persistance_simd, per_simd);
                per_simd = _mm256_srai_epi32(per_simd, INT_ONE_BIT_COUNT);
            }

            __m256i* output = (__m256i*)out_noise + index;
            _mm256_store_si256(output, noise);
        }
    }
}

/**
 * @brief Simplex noise using integers instead of floats. INT_ONE maps to 1.0
 *
 * @param pos Position in integer space
 * @return Simplex noise value
 */
void simplex_2d_int_simd(s32 seed, vec2i pos, vec2i size, s32 scale, s32* out_noise) {
    SASSERT(((size_t)out_noise & 31) == 0, "Cannot write simplex data to noise buffer: Buffer not aligned to 0x20 bytes (%p)", out_noise);
    // Constants
    const __m256i scale_simd = _mm256_set1_epi32(scale);
    const s32 add_vector[8] __attribute__((aligned (32))) = { 0, 1, 2, 3, 4, 5, 6, 7 };
    const u32 simd_size = sizeof(__m256i) / 4;

    __m256i add = _mm256_stream_load_si256((const void*)add_vector);
    add = _mm256_mullo_epi32(add, scale_simd);

    for (u32 _y = 0, index = 0; _y < size.y; _y++) {
        const __m256i y = _mm256_set1_epi32((pos.y + _y) * scale);
        for (u32 _x = 0; _x < size.x; _x += simd_size, index++) {
            const __m256i x = _mm256_add_epi32(add, _mm256_set1_epi32((pos.x + _x) * scale));

            __m256i* output = (__m256i*)out_noise + index;
            simplex_2d_int_simd_row(seed, x, y, output);
        }
    }
}
void simplex_2d_int_simd_row(const u32 seed, __m256i x, __m256i y, __m256i* output) {
    // Constant values
    const s32 noise_multiplier = (38.283687591552734375 * INT_ONE);
    const __m256i x_prime = _mm256_set1_epi32(X_PRIME);
    const __m256i y_prime = _mm256_set1_epi32(Y_PRIME);
    const __m256i INT_ONE_FLOOR_MASK = _mm256_set1_epi32(~((1 << INT_ONE_BIT_COUNT) - 1));
    const __m256i SIMD_G2              = _mm256_set1_epi32((s32)(G2 * INT_ONE));
    const __m256i SIMD_INT_ONE         = _mm256_set1_epi32(INT_ONE);
    const __m256i SIMD_TWO_G2_MINUS_ONE = _mm256_set1_epi32((s32)((G2 * 2 - 1) * INT_ONE));

    // Calculate skew and grid position
    const __m256i pos_sum = _mm256_add_epi32(x, y);
    __m256i f = _mm256_set1_epi32((s32)(F2 * INT_ONE));
    f = _mm256_mullo_epi32(f, pos_sum);
    f = _mm256_srai_epi32(f, INT_ONE_BIT_COUNT);

    __m256i x0 = _mm256_add_epi32(x, f);
    x0 = _mm256_and_si256(x0, INT_ONE_FLOOR_MASK);
    __m256i y0 = _mm256_add_epi32(y, f);
    y0 = _mm256_and_si256(y0, INT_ONE_FLOOR_MASK);

    __m256i i = _mm256_mullo_epi32(_mm256_srai_epi32(x0, INT_ONE_BIT_COUNT), x_prime);
    __m256i j = _mm256_mullo_epi32(_mm256_srai_epi32(y0, INT_ONE_BIT_COUNT), y_prime);

    __m256i g = _mm256_add_epi32(x0, y0);
    g = _mm256_mullo_epi32(SIMD_G2, g);
    g = _mm256_srai_epi32(g, INT_ONE_BIT_COUNT);

    // Calculate x0,x1,x2, y0,y1,y2
    x0 = _mm256_sub_epi32(x, _mm256_sub_epi32(x0, g));
    y0 = _mm256_sub_epi32(y, _mm256_sub_epi32(y0, g));

    __m256i i1 = _mm256_cmpgt_epi32(x0, y0);

    // s32 x1 = i1 ? x0 - INT_ONE : x0;
    // s32 y1 = i1 ? y0           : y0 - INT_ONE;
    __m256i x1 = _mm256_blendv_epi8(x0, _mm256_sub_epi32(x0, SIMD_INT_ONE), i1);
    x1 = _mm256_add_epi32(x1, SIMD_G2);
    __m256i y1 = _mm256_blendv_epi8(_mm256_sub_epi32(y0, SIMD_INT_ONE), y0, i1);
    y1 = _mm256_add_epi32(y1, SIMD_G2);

    __m256i x2 = _mm256_add_epi32(x0, SIMD_TWO_G2_MINUS_ONE);
    __m256i y2 = _mm256_add_epi32(y0, SIMD_TWO_G2_MINUS_ONE);

    // Calculate interpolation values
    __m256i t0 = simd_calculate_t(x0, y0);
    __m256i t1 = simd_calculate_t(x1, y1);
    __m256i t2 = simd_calculate_t(x2, y2);

    // Calculate noise components
    __m256i n0 = simd_get_gradient_dot_fancy(
            simd_hash_primes(seed, i, j), 
            x0, y0);

    __m256i n1 = simd_get_gradient_dot_fancy(
            simd_hash_primes(seed, 
                _mm256_blendv_epi8(i, _mm256_add_epi32(i, x_prime), i1), 
                _mm256_blendv_epi8(_mm256_add_epi32(j, y_prime), j, i1)), 
            x1, y1);

    __m256i n2 = simd_get_gradient_dot_fancy(
            simd_hash_primes(seed,
                _mm256_add_epi32(i, x_prime),
                _mm256_add_epi32(j, y_prime)), 
            x2, y2);

    n0 = _mm256_srai_epi32(_mm256_mullo_epi32(n0, t0), INT_ONE_BIT_COUNT);
    n1 = _mm256_srai_epi32(_mm256_mullo_epi32(n1, t1), INT_ONE_BIT_COUNT);
    n2 = _mm256_srai_epi32(_mm256_mullo_epi32(n2, t2), INT_ONE_BIT_COUNT);

    // Calculate noise
    __m256i noise = _mm256_add_epi32(_mm256_add_epi32(n0, n1), n2);
    noise = _mm256_mullo_epi32(noise, _mm256_set1_epi32(noise_multiplier));
    noise = _mm256_srai_epi32 (noise, INT_ONE_BIT_COUNT);

    // Store result
    _mm256_store_si256(output, noise);
}

// =========================================
// Private functions
// =========================================
f32 calculate_t(f32 x, f32 y) {
    s32 _y = -(y * y);
    s32 _x = -(x * x);
    s32 t = _x + _y + .5f;
    if (t <= 0) {
        return 0;
    }

    t = t * t;
    t = t * t;
    return t;
}

s32 calculate_t_int(s32 x, s32 y) {
    s32 _y = (-(y * y)) >> INT_ONE_BIT_COUNT;
    s32 _x = (-(x * x)) >> INT_ONE_BIT_COUNT;
    s32 t = _x + _y + INT_ONE / 2;
    if (t <= 0) {
        return 0;
    }

    t = (t * t) >> INT_ONE_BIT_COUNT;
    t = (t * t) >> INT_ONE_BIT_COUNT;
    return t;
}

__m256i simd_calculate_t(__m256i x, __m256i y) {
    const __m256i SIMD_HALF = _mm256_set1_epi32(INT_ONE / 2);
    const __m256i SIMD_ZERO = _mm256_set1_epi32(0);

    __m256i negative = _mm256_set1_epi32(-1);
    __m256i x_sqr = _mm256_mullo_epi32(x, x);
    x_sqr = _mm256_srai_epi32(x_sqr, INT_ONE_BIT_COUNT);
    __m256i y_sqr = _mm256_mullo_epi32(y, y);
    y_sqr = _mm256_srai_epi32(y_sqr, INT_ONE_BIT_COUNT);

    y_sqr = _mm256_sign_epi32(y_sqr, negative);
    x_sqr = _mm256_sign_epi32(x_sqr, negative);

    __m256i t = _mm256_add_epi32(SIMD_HALF, _mm256_add_epi32(x_sqr, y_sqr));
    t = _mm256_max_epi32(t, SIMD_ZERO);

    t = _mm256_mullo_epi32(t, t);
    t = _mm256_srai_epi32(t, INT_ONE_BIT_COUNT);
    t = _mm256_mullo_epi32(t, t);
    t = _mm256_srai_epi32(t, INT_ONE_BIT_COUNT);

    return t;
}

s32 hash_primes(u32 seed, s32 a, s32 b) {
    s32 hash = seed ^ (a ^ b);

    hash *= 0x27d4eb2d;
    return (hash >> 15) ^ hash;
}

__m256i simd_hash_primes(u64 seed, __m256i a, __m256i b) {
    const __m256i mul = _mm256_set1_epi32(0x27d4eb2d);

    __m256i hash = _mm256_xor_si256(_mm256_set1_epi32(seed), _mm256_xor_si256(a, b));

    hash = _mm256_mullo_epi32(hash, mul);
    // hash = _mm256_xor_si256(hash, _mm256_srai_epi32(hash, 15));
    return hash;
}

f32 get_gradient_dot_fancy(s32 hash, f32 fX, f32 fY) {
    s32 index = (hash & 0x3FFFFF) * (1.0 / 1.333333333333333333333333);

    // Bit-4 = Choose X Y ordering
    s32 xy = (index & ( 1 << 2 )) != 0;

    f32 a = fX;
    f32 b = fY;
    if (xy) {
        a = fY;
        b = fX;
    }

    // Bit-1 = b flip sign
    b *= -1;

    // Bit-2 = Mul a by 2 or Root3
    s32 amul2 = ( index & ( 1 << 1 ) ) != 0;

    if (amul2) {
        a *= 2;
    } else {
        a *= S_SQRT_THREE;
    }
    b = b * !amul2;

    return (a + b);
}

s32 get_gradient_dot_fancy_int(s32 hash, s32 fX, s32 fY ) {
    const s32 mul = INT_ONE / 1.3333333333333333333333f;

    s32 index = ((hash & 0x3FFFFF) * mul) >> INT_ONE_BIT_COUNT;

    // Bit-4 = Choose X Y ordering
    s32 xy = (index & ( 1 << 2 )) != 0;

    s32 a = fX;
    s32 b = fY;
    if (xy) {
        a = fY;
        b = fX;
    }

    // Bit-1 = b flip sign
    b *= -1;

    // Bit-2 = Mul a by 2 or Root3
    s32 amul2 = ( index & ( 1 << 1 ) ) != 0;

    if (amul2) {
        a *= 2;
    } else {
        a *= (s32)(INT_ONE * S_SQRT_THREE);
        a >>= INT_ONE_BIT_COUNT;
    }
    b = b * !amul2;

    // Bit-8 = Flip sign of a + b
    return (a + b);
}

SINLINE __m256i simd_get_gradient_dot_fancy(__m256i hash, __m256i fx, __m256i fy) {

    //     int32v index = FS_Convertf32_i32( FS_Converti32_f32( hash & int32v( 0x3FFFFF ) ) * float32v( 1.3333333333333333f ) );
    //
    // float32v gX = _mm256_permutevar8x32_ps( float32v( ROOT3, ROOT3, 2, 2, 1, -1, 0, 0 ), index );
    // float32v gY = _mm256_permutevar8x32_ps( float32v( 1, -1, 0, 0, ROOT3, ROOT3, 2, 2 ), index );
    //
    // // Bit-8 = Flip sign of a + b
    // return FS_FMulAdd_f32( gX, fX, fY * gY ) ^ FS_Casti32_f32( (index >> 3) << 31 );
    //
    //
    // __m256i SIMD_ZERO = _mm256_set1_epi32(0);
    //
    __m256i index = hash;
    index = _mm256_srai_epi32(index, INT_ONE_BIT_COUNT);

    const s32 root3 = S_SQRT_THREE * INT_ONE;
    __m256i gx = _mm256_permutevar8x32_epi32(_mm256_setr_epi32(root3, root3, INT_ONE * 2, INT_ONE * 2, INT_ONE, -INT_ONE, 0, 0), index);
    __m256i gy = _mm256_permutevar8x32_epi32(_mm256_setr_epi32(INT_ONE, -INT_ONE, 0, 0, root3, root3, 2 * INT_ONE, 2 * INT_ONE), index);

    __m256i _x = _mm256_srai_epi32(_mm256_mullo_epi32(gx, fx), INT_ONE_BIT_COUNT);
    __m256i _y = _mm256_srai_epi32(_mm256_mullo_epi32(gy, fy), INT_ONE_BIT_COUNT);
    return _mm256_add_epi32(_y, _x);
}

s32 fast_floor(const f32 x) {
    return x > 0 ? (int)x : (int)x - 1;
}
