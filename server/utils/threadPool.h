#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "logger.h"

extern Logger logger;

#define MAX_THREADS 10
#define MAX_TASKS 100

typedef struct Task {
    void (*task)(void* arg);
    void* arg;
} Task;

typedef struct  ThreadPool {
    Task* tasks[MAX_TASKS];
    int front;
    int rear;
    int count;
    pthread_mutex_t mutex;
    pthread_mutex_t not_empty_mutex; // New mutex for not_empty condition variable
    pthread_mutex_t all_tasks_completed_mutex; // New mutex for all_tasks_completed condition variable
    pthread_mutex_t not_full_mutex; // New mutex for not_full condition variable
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    int shutdown; // New shutdown flag
    int pending_tasks; // New counter for pending tasks
    pthread_cond_t all_tasks_completed; // New condition variable for waiting on all tasks to complete
} ThreadPool;

/**
 * @brief Initialize the thread pool
 * 
 * @param pool The thread pool to be initialized
 */
void init_pool(ThreadPool* pool);

/**
 * @brief Destroy the thread pool
 * 
 * @param pool The thread pool to be destroyed
 */
void destroy_pool(ThreadPool* pool);

/**
 * @brief Enqueue a task to the thread pool
 * 
 * @param pool The thread pool
 * @param task The task to be enqueued
 */
void enqueue_task(ThreadPool* pool, Task* task);

/**
 * @brief Dequeue a task from the thread pool
 * 
 * @param pool The thread pool
 * @return Task* The dequeued task
 */
Task* dequeue_task(ThreadPool* pool);

/**
 * @brief The worker thread
 * 
 * @param arg The thread pool
 * @return void* 
 */
void* worker(void* arg);

/**
 * @brief Submit a task to the thread pool
 * 
 * @param pool The thread pool
 * @param task The task to be submitted
 * @param arg The argument to the task
 */
void submit_task(ThreadPool* pool, void (*task)(void* arg), void* arg);

/**
 * @brief Shutdown the thread pool
 * 
 * @param pool The thread pool
 */ 
void shutdown_pool(ThreadPool* pool);

#endif