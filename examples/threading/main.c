#include <pthread.h>
#include "threading.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


int main(int argc, char *argv[]) {
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    
    pthread_t thread;
    int wait_to_obtain_ms = 1000; // 1 second
    int wait_to_release_ms = 500; // 0.5 seconds

    if (start_thread_obtaining_mutex(&thread, &mutex, wait_to_obtain_ms, wait_to_release_ms)) {
        struct thread_data *result;
        pthread_join(thread, (void**)&result);
        if (result) {
            printf("Thread completed with success: %s\n", result->thread_complete_success ? "true" : "false");
            free(result);
        }
    } else {
        printf("Failed to start thread.\n");
    }
    
    printf("Running 2 threads with different wait times...\n");
    pthread_t thread1, thread2;
    if (start_thread_obtaining_mutex(&thread1, &mutex, 1000, 1000) &&
        start_thread_obtaining_mutex(&thread2, &mutex, 250, 250)) {
        struct thread_data *result1, *result2;
        pthread_join(thread1, (void**)&result1);
        pthread_join(thread2, (void**)&result2);
        
        if (result1) {
            printf("Thread 1 completed with success: %s\n", result1->thread_complete_success ? "true" : "false");
            free(result1);
        }
        if (result2) {
            printf("Thread 2 completed with success: %s\n", result2->thread_complete_success ? "true" : "false");
            free(result2);
        }
    } else {
        printf("Failed to start one or both threads.\n");
    }
    pthread_mutex_destroy(&mutex);
    return 0;
}