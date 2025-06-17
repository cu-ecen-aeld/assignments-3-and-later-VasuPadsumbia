#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,... ) printf("threading: " msg "\n" , ##__VA_ARGS__)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    if (thread_func_args == NULL) {
        ERROR_LOG("Invalid thread parameters");
        return NULL;
    }
    DEBUG_LOG("Thread started with wait_to_obtain_ms=%d, wait_to_release_ms=%d",
              thread_func_args->wait_to_obtain_ms,
              thread_func_args->wait_to_release_ms);    
    // Sleep for the specified time before trying to obtain the mutex
    usleep(thread_func_args->wait_to_obtain_ms * 1000); // Convert ms to us
    DEBUG_LOG("Thread %ld attempting to obtain mutex", thread_func_args->thread_id);
    int ret = pthread_mutex_lock(thread_func_args->mutex);
    if (ret != 0) {
        ERROR_LOG("Thread failed to obtain mutex: %s", strerror(ret));
        thread_func_args->thread_complete_success = false;
        return NULL;
    }
    DEBUG_LOG("Thread obtained mutex");
    usleep(thread_func_args->wait_to_release_ms * 1000); // Convert ms to us
    DEBUG_LOG("Thread %ld releasing mutex", thread_func_args->thread_id);

    ret = pthread_mutex_unlock(thread_func_args->mutex);
    if (ret != 0) {
        ERROR_LOG("Thread failed to release mutex: %s", strerror(ret));
        thread_func_args->thread_complete_success = false;
        return NULL;
    }
    DEBUG_LOG("Thread released mutex");
    thread_func_args->thread_complete_success = true;
    DEBUG_LOG("Thread completed successfully");
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    struct thread_data *data = malloc(sizeof(struct thread_data));
    if (data == NULL) {
        ERROR_LOG("Failed to allocate memory for thread_data");
        return false;
    }
    data->mutex = mutex;
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    data->thread_complete_success = false;
    data->thread_id = pthread_self(); // Store the thread ID for debugging
    DEBUG_LOG("Starting thread with wait_to_obtain_ms=%d, wait_to_release_ms=%d",
              wait_to_obtain_ms, wait_to_release_ms);
    int ret = pthread_create(thread, NULL, threadfunc, (void *)data);
    if (ret != 0) {
        ERROR_LOG("Failed to create thread: %s", strerror(ret));
        free(data);
        return false;
    }

    data->thread_id = *thread; // Update the thread ID in the data structure    
    DEBUG_LOG("Thread %ld started successfully", data->thread_id);
    DEBUG_LOG("Data from the thread: wait_to_obtain_ms=%d, wait_to_release_ms=%d, thread_complete_success=%d",
              data->wait_to_obtain_ms, data->wait_to_release_ms, data->thread_complete_success);
    // The thread has been started successfully, return true
    return true;
}

