#include "Spark/threading/mutex.h"
#include "Spark/core/smemory.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>

#define MAX_MUTEX_COUNT 4096

typedef struct {
    pthread_mutex_t mutex;
} internal_mutex_t;

void mutex_create(spark_mutex_t* out_mutex) {
    internal_mutex_t* mutex = sallocate(sizeof(internal_mutex_t), MEMORY_TAG_THREAD);
    pthread_mutex_init(&mutex->mutex, NULL);
    out_mutex->internal_data = mutex;
    SDEBUG("Creating mutex %p", mutex);
}

void mutex_destroy(spark_mutex_t* out_mutex) {
    if (out_mutex->internal_data) {
        sfree(out_mutex->internal_data, sizeof(internal_mutex_t), MEMORY_TAG_THREAD);
        SDEBUG("Destroying mutex %p", out_mutex->internal_data);
        return;
    }

    SERROR("Failed to destroy mutex: %p", out_mutex->internal_data);
}

void mutex_lock(spark_mutex_t mutex) {
    internal_mutex_t* _mutex = mutex.internal_data;
    pthread_mutex_lock(&_mutex->mutex);
}

void mutex_unlock(spark_mutex_t mutex) {
    internal_mutex_t* _mutex = mutex.internal_data;
    pthread_mutex_unlock(&_mutex->mutex);
}

