#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
// #define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    // Wait, set the return value, obtain the mutex, wait and return the pointer to the shared memory struct!
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    thread_func_args->thread_complete_success = true;


    DEBUG_LOG("Sleeping %d milliseconds to lock", thread_func_args->wait_to_obtain_ms);

    usleep(thread_func_args->wait_to_obtain_ms * 1000);

    int rc = pthread_mutex_lock(thread_func_args->mutex);

    if (rc != 0) {
        thread_func_args->thread_complete_success = false;
    }

    DEBUG_LOG("Sleeping %d milliseconds to unlock", thread_func_args->wait_to_release_ms);

    usleep(thread_func_args->wait_to_release_ms * 1000);

    rc = pthread_mutex_unlock(thread_func_args->mutex);

    if (rc != 0) {
        thread_func_args->thread_complete_success = false;
        printf("return code error !!\n");
    }

    return thread_param;

}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{

    struct thread_data *thread_param = malloc(sizeof(struct thread_data));

    thread_param->mutex = mutex;

    thread_param->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_param->wait_to_release_ms = wait_to_release_ms;

    /* The pthread_create() call stores the thread ID into
        corresponding element of tinfo[]. */

    int s = pthread_create(thread, NULL,
                        &threadfunc, thread_param);
    if (s != 0) {
        return false;
    }

    return true;
}

