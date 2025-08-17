#include "Spark/threading/thread.h"
#include "Spark/defines.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#define MAX_THREAD_COUNT 512

typedef struct { 
    pthread_t thread;
    thread_state_t state;
} internal_thread_t;

internal_thread_t internal_threads[MAX_THREAD_COUNT];

void thread_create(void (*function(void* args)), void* args, thread_t* out_thread) {
    // Find valid thread ID
    u32 thread_id = INVALID_ID;
    for (u32 i = 0; i < MAX_THREAD_COUNT; i++) {
        if (internal_threads[i].state == THREAD_STATE_INVALID || internal_threads[i].state == THREAD_STATE_COMPLETE) {
            thread_id = i;
            break;
        }
    }

    if (thread_id == INVALID_ID) {
        SERROR("Failed to find open thread.");
        out_thread->thread_id = INVALID_ID;
        return;
    }

    // Remember the current internal thread state
    internal_threads[thread_id].state = THREAD_STATE_RUNNING;

    // Set the out thread information
    out_thread->thread_id = thread_id;

    // Start the thread
    pthread_create(&internal_threads[thread_id].thread, NULL, function, args);
}

void thread_destroy(thread_t* thread) {
    SASSERT(thread->thread_id != INVALID_ID, "Cannot destroy invalid thread.");
    internal_threads[thread->thread_id].state = THREAD_STATE_INVALID;
}

void thread_join(thread_t thread) {
    pthread_join(internal_threads[thread.thread_id].thread, NULL);
}

void thread_abort(thread_t thread) {
    pthread_cancel(internal_threads[thread.thread_id].thread);
    pthread_kill(internal_threads[thread.thread_id].thread, 0);
}

void thread_sleep(thread_t* thread, u32 time_us) {
    usleep(time_us);
}
