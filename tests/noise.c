#include "Spark/core/logging.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
     // int stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);
#include "Spark/random/noise/simplex.h"
#include "stb_image_write.h"

#define SAMPLE_SIZE 512

    s32 noise[SAMPLE_SIZE * SAMPLE_SIZE];
    u8 pixels[SAMPLE_SIZE * SAMPLE_SIZE];
    char line[SAMPLE_SIZE + 1] = {};
int main() {

    // simplex_2d_int_simd((vec2i) { }, (vec2i) {SAMPLE_SIZE, SAMPLE_SIZE}, (void*)noise);

    const f32 float_scale = .01f;
    const s32 int_scale = 1;
    int i = 0;
    for (u32 y = 0; y < SAMPLE_SIZE; y++) {
        for (u32 x = 0; x < SAMPLE_SIZE; x++) {
            vec2i posi = { .x = (x - SAMPLE_SIZE / 2) * int_scale, (y - SAMPLE_SIZE / 2) * int_scale };
            s32 value = simplex_2d_int(0, posi);
            if (value < 0) {
                value *= -1;
            }
            pixels[i] = (u8)(value >> 2);
            // SDEBUG("Value: %d (%d, %f)", value, (u8)(value >> 6), (float)value / 4096);
            // vec2 pos = {.x = x * float_scale, .y = y * float_scale };
            // f32 value = simplex_2d(pos);
            // pixels[i] = (u8)(value * 255);
            i++;
        }

        // SDEBUG("Line: %s", line);
    }

    stbi_write_png("Int2D_Simd.png", SAMPLE_SIZE, SAMPLE_SIZE, 1, pixels, SAMPLE_SIZE);
}
