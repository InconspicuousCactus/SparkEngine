#include "Spark/math/smath.h"
#include "Spark/platform/platform.h"

#include <math.h>
#include <stdlib.h>

static b8 rand_seeded = false;

/**
 * Note that these are here in order to prevent having to import the
 * entire <math.h> everywhere.
 */
f32 ssin(f32 x) {
    return sinf(x);
}

f32 scos(f32 x) {
    return cosf(x);
}

f32 stan(f32 x) {
    return tanf(x);
}

f32 sacos(f32 x) {
    return acosf(x);
}

f32 ssqrt(f32 x) {
    return sqrtf(x);
}

f32 sabs(f32 x) {
    return fabsf(x);
}

s32 absi(s32 x) {
    return abs(x);
}

s32 s_random() {
    if (!rand_seeded) {
        srand((u32)platform_get_absolute_time());
        rand_seeded = true;
    }
    return rand();
}

s32 s_random_in_range(s32 min, s32 max) {
    if (!rand_seeded) {
        srand((u32)platform_get_absolute_time());
        rand_seeded = true;
    }
    return (rand() % (max - min + 1)) + min;
}

f32 sf_random() {
    return (float)s_random() / (f32)RAND_MAX;
}

f32 fkrandom_in_range(f32 min, f32 max) {
    return min + ((float)s_random() / ((f32)RAND_MAX / (max - min)));
}
