#pragma once
#include "Spark/defines.h"

typedef enum : u8 {
    JOB_STATE_EMPTY,
    JOB_STATE_INITIALIZED,
    JOB_STATE_QUEUED,
    JOB_STATE_RUNNING,
    JOB_STATE_COMPLETE,
    JOB_STATE_DESTROYED,
} job_state_t;

typedef struct {
    void (*job_function)(void* arg);
    void (*complete_callback)();
    void* args;
    u32 arg_size;
    s16 job_priority;
    job_state_t state;
} job_t;

/**
 * @brief Adds a job to the list of jobs to be completed.
 *
 * @param job The job info
 * @return Index of the job handle. INVALID_ID if failed to add job
 */
u32 job_system_add(job_t* job);

