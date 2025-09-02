#include "Spark/utils/benchmarking.h"
#include "Spark/core/clock.h"
#include "Spark/core/smemory.h"

const char* timescale_strings[] = {
    "s",
    "ms",
    "us"
};
const u32 timescale_multiplier[] = {
    1, 
    1000,
    1000 * 1000,
};

s32 sort_f32(const void* _a, const void* _b) {
    const f32* a = _a;
    const f32* b = _b;

    if (*a < *b)
        return -1;
    if (*a > *b) 
        return 1;
    return 0;
}

void benchmark_function(const char* name, u32 iteration_count, time_scale_t timescale, void (function(spark_clock_t* clock, void* args)), void* args) {
    // Setup
    f32* times = sallocate(iteration_count * sizeof(f32), MEMORY_TAG_ARRAY);
    spark_clock_t clock;

    // Run function
    for (u32 i = 0; i < iteration_count; i++) {
        function(&clock, args);
        times[i] = clock.elapsed_time;
    }

    // Analyze statistics
    qsort(times, iteration_count, sizeof(f32), sort_f32);
    f32 min = times[0];
    f32 max = times[iteration_count - 1];
    f32 median = times[iteration_count / 2];
    f32 average = 0;
    for (u32 i = 0; i < iteration_count; i++) {
        average += times[i];
    }
    f32 total = average;
    average /= iteration_count;

    min *= timescale_multiplier[timescale];
    max *= timescale_multiplier[timescale];
    median *= timescale_multiplier[timescale];
    average *= timescale_multiplier[timescale];

    const char* timescale_string = timescale_strings[timescale];
    SERROR("Benchmark '%s':\n\tIterations: %d\n\tTotal  : %fs\n\tMin    : %f%s\n\tMax    : %f%s\n\tMeidan : %f%s\n\tAverage: %f%s\n\tSpeed  : %f iterations/second", name, iteration_count, 
            total,
            min,     timescale_string,
            max,     timescale_string,
            median,  timescale_string,
            average, timescale_string,
            iteration_count / total);
    
    // Shutdown
    sfree(times, iteration_count * sizeof(f32), MEMORY_TAG_ARRAY);
}


