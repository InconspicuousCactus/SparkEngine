#pragma once

#include "Spark/core/clock.h"
#include "Spark/defines.h"

typedef enum time_scale {
    TIMESCALE_SECONDS,
    TIMESCALE_MILLI_SECONDS,
    TIMESCALE_MICRO_SECONDS,
} time_scale_t;

void benchmark_function(const char* name, u32 iteration_count, time_scale_t timescale, void (function(spark_clock_t* clock, void* args)), void* args);
