#pragma once

typedef enum : u8 {
    THREAD_STATE_INVALID,
    THREAD_STATE_CREATED,
    THREAD_STATE_RUNNING,
    THREAD_STATE_COMPLETE,
} thread_state_t;

typedef struct {
    u32 thread_id;
} thread_t;

/**
 * @brief Creates and starts a function
 *
 * @param function The function the thread starts in
 * @param args Arguments for the function
 * @param out_thread Output for the created thread
 */
void thread_create(void (*function(void* args)), void* args, thread_t* out_thread);
void thread_destroy(thread_t* thread);

void thread_sleep(thread_t* thread, u32 time_us);

void thread_join(thread_t thread);
void thread_abort(thread_t thread);
