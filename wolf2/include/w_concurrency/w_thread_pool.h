/*
    Project          : Wolf Engine. Copyright(c) Pooya Eimandar (https://PooyaEimandar.github.io) . All rights reserved.
    Source           : Please direct any bug to https://github.com/WolfEngine/Wolf.Engine/issues
    Website          : https://WolfEngine.App
    Name             : w_concurrent_queue.h
    Description      : a thread pool
    Comment          : 
*/

#pragma once

#include <wolf.h>
#include "w_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct apr_thread_pool apr_thread_pool_t;
    typedef apr_thread_pool_t* w_thread_pool;


#define W_THREAD_TASK_PRIORITY_LOWEST 0
#define W_THREAD_TASK_PRIORITY_LOW 63
#define W_THREAD_TASK_PRIORITY_NORMAL 127
#define W_THREAD_TASK_PRIORITY_HIGH 191
#define W_THREAD_TASK_PRIORITY_HIGHEST 255

    /**
     * initialize a thread pool
     * @param pMemPool The pool to use
     * @param pThreadPool The pointer in which to return the newly created apr_thread_pool object, or NULL if thread pool creation fails.
     * @param pInitThreads The number of threads to be created initially, this number will also be used as the initial value for the maximum number of idle threads.
     * @param pMaxThreads The maximum number of threads that can be created
     * @returns APR_BADARG on bad functions arguments
     * @returns APR_SUCCESS if the thread pool was created successfully. Otherwise, the error code.
     */
    W_SYSTEM_EXPORT
        W_RESULT w_thread_pool_init(
            _Inout_ w_mem_pool pMemPool,
            _Inout_ w_thread_pool* pThreadPool,
            _In_ size_t pInitThreads,
            _In_ size_t pMaxThreads);

    /**
     * Schedule a task to the bottom of the tasks of same priority.
     * @param me The thread pool
     * @param func The task function
     * @param param The parameter for the task function
     * @param priority The priority of the task.
     * @param owner Owner of this task.
     * @returns APR_BADARG on bad functions arguments
     * @returns APR_SUCCESS if the task had been scheduled successfully
     */
    W_SYSTEM_EXPORT
        W_RESULT w_thread_pool_push(
            _In_ w_thread_pool pThreadPool,
            _In_ w_thread_job pThreadJob,
            _In_opt_ void* pParam,
            _In_ uint8_t pPriority,
            _In_opt_ void* pOwner);

    /**
     * Schedule a task to be run after a delay
     * @param pThreadPool The thread pool
     * @param pThreadJob The task function
     * @param pParam The parameter for the task function
     * @param pTime Time in microseconds
     * @param pOwner Owner of this task
     * @returns APR_BADARG on bad functions arguments
     * @returns APR_SUCCESS if the task had been scheduled successfully
     */
    W_SYSTEM_EXPORT
        W_RESULT w_thread_pool_schedule(
            _In_ w_thread_pool pThreadPool,
            _In_ w_thread_job pThreadJob,
            _In_opt_ void* pParam,
            _In_ int64_t pTime,
            _In_opt_ void* pOwner);

    /**
     * Schedule a task to the top of the tasks of same priority.
     * @param pThreadPool The thread pool
     * @param pThreadJob The task function
     * @param pParam The parameter for the task function
     * @param pPriority The priority of the task
     * @param pOwner Owner of this task
     * @returns APR_BADARG on bad functions arguments
     * @returns APR_SUCCESS if the task had been scheduled successfully
     */
    W_SYSTEM_EXPORT
        W_RESULT w_thread_pool_top(
            _In_ w_thread_pool pThreadPool,
            _In_ w_thread_job pThreadJob,
            _In_opt_ void* pParam,
            _In_ uint8_t pPriority,
            _In_ void* pOwner);

    /**
     * Cancel tasks submitted by the owner. If there is any task from the owner that
     * is currently running, the function will spin until the task finished.
     * @param pThreadPool The thread pool
     * @param pOwner Owner of the task
     * @returns APR_BADARG on bad functions arguments
     * @returns APR_SUCCESS if the task has been cancelled successfully
     * @note The task function should not be calling cancel, otherwise the function
     * may get stuck forever. The function assert if it detect such a case.

     */
    W_SYSTEM_EXPORT
        W_RESULT w_thread_pool_tasks_cancel(_In_ w_thread_pool pThreadPool, _In_ void* pOwner);

    /**
     * Get the current number of tasks waiting in the queue
     * @param pThreadPool The thread pool
     * @return Number of tasks in the queue
     */
    W_SYSTEM_EXPORT
        size_t w_thread_pool_tasks_count(_In_ w_thread_pool pThreadPool);

    /**
     * Get the current number of scheduled tasks waiting in the queue
     * @param pThreadPool The thread pool
     * @return Number of scheduled tasks in the queue
     */
    W_SYSTEM_EXPORT
        size_t w_thread_pool_scheduled_tasks_count(_In_ w_thread_pool pThreadPool);

    /**
     * Get the current number of threads
     * @param pThreadPool The thread pool
     * @return Total number of threads
     */
    W_SYSTEM_EXPORT
        size_t w_thread_pool_threads_count(_In_ w_thread_pool pThreadPool);

    /**
     * Get the current number of busy threads
     * @param me The thread pool
     * @return Number of busy threads
     */
    W_SYSTEM_EXPORT
        size_t w_thread_pool_busy_count(_In_ w_thread_pool pThreadPool);

    /**
     * Get the current number of idle threads
     * @param pThreadPool The thread pool
     * @return Number of idle threads
     */
    W_SYSTEM_EXPORT
        size_t w_thread_pool_idle_count(_In_ w_thread_pool pThreadPool);

    /**
     * Access function for the maximum number of idle threads. Number of current
     * idle threads will be reduced to the new limit.
     * @param pThreadPool The thread pool
     * @param pCount The number
     * @return The number of threads that were stopped.
     */
    W_SYSTEM_EXPORT
        size_t w_thread_pool_idle_max_set(
            _In_ w_thread_pool pThreadPool,
            _In_ size_t pCount);

    /**
     * Get number of tasks that have run
     * @param pThreadPool The thread pool
     * @return Number of tasks that have run
     */
    W_SYSTEM_EXPORT
        size_t w_thread_pool_tasks_run_count(_In_ w_thread_pool pThreadPool);

    /**
     * Get high water mark of the number of tasks waiting to run
     * @param me The thread pool
     * @return High water mark of tasks waiting to run
     */
    W_SYSTEM_EXPORT
        size_t w_thread_pool_tasks_high_count(_In_ w_thread_pool pThreadPool);

    /**
     * Get high water mark of the number of threads
     * @param pThreadPool The thread pool
     * @return High water mark of threads in thread pool
     */
    W_SYSTEM_EXPORT
        size_t w_thread_pool_threads_high_count(_In_ w_thread_pool pThreadPool);

    /**
     * Get the number of idle threads that were destroyed after timing out
     * @param me The thread pool
     * @return Number of idle threads that timed out
     */
    W_SYSTEM_EXPORT
        size_t w_thread_pool_threads_idle_timeout_count(_In_ w_thread_pool pThreadPool);

    /**
     * Access function for the maximum number of idle threads
     * @param pThreadPool The thread pool
     * @return The current maximum number
     */
    W_SYSTEM_EXPORT
        size_t w_thread_pool_idle_max_get(_In_ w_thread_pool pThreadPool);

    /**
     * Access function for the maximum wait time (in microseconds) of an
     * idling thread that exceeds the maximum number of idling threads
     * @param pThreadPool The thread pool
     * @return The current maximum wait time
     */
    W_SYSTEM_EXPORT
        int64_t w_thread_pool_idle_wait_get(_In_ w_thread_pool pThreadPool);

    /**
     * Access function for the maximum number of threads
     * @param pThreadPool The thread pool
     * @return The current maximum number
     */
    W_SYSTEM_EXPORT
        size_t w_thread_pool_thread_max_get(_In_ w_thread_pool pThreadPool);

    /**
     * Access function for the maximum number of threads.
     * @param pThreadPool The thread pool
     * @param pCount Number of threads
     * @return The original maximum number of threads
     */
    W_SYSTEM_EXPORT
        size_t w_thread_pool_thread_max_set(
            _In_ w_thread_pool pThreadPool,
            _In_ size_t pCount);

    /**
     * Access function for the maximum wait time (in microseconds) of an idling 
     * thread that exceeds the maximum number of idling threads.
     * A non-zero value allows for the reaping of idling threads to shrink
     * over time.  Which helps reduce thrashing.
     * @param pThreadPool The thread pool
     * @param pTimeout The number of microseconds an idle thread should wait
     * till it reaps itself
     * @return The original maximum wait time
     */
    W_SYSTEM_EXPORT
        int64_t w_thread_pool_idle_wait_set(
            _In_ w_thread_pool pThreadPool,
            _In_ int64_t pTimeout);

    /**
     * Access function for the threshold of tasks in queue to trigger a new thread.
     * @param pThreadPool The thread pool
     * @return The current threshold
     */
    W_SYSTEM_EXPORT
        size_t w_thread_pool_threshold_get(_In_ w_thread_pool pThreadPool);

    /**
     * Access function for the threshold of tasks in queue to trigger a new thread.
     * @param pThreadPool The thread pool
     * @param pValue The new threshold
     * @return The original threshold
     */
    W_SYSTEM_EXPORT
        size_t w_thread_pool_threshold_set(
            _In_ w_thread_pool pThreadPool,
            _In_ size_t pValue);

    /**
     * Get owner of the task currently been executed by the thread.
     * @param pThread The thread is executing a task
     * @param owner Pointer to receive owner of the task.
     * @return APR_SUCCESS if the owner is retrieved successfully
     */
    W_SYSTEM_EXPORT
        W_RESULT w_thread_pool_task_owner_get(
            _In_ w_thread pThread,
            _In_ void** pOwner);

    /**
     * Wait for specific threads to execute their jobs.
     * @param pThreadPool The thread pools
     * @param pNumberOfTasks The number of tasks
     */
    W_SYSTEM_EXPORT
        void w_thread_pool_wait_for(
            _Inout_ w_thread_pool pThreadPool,
            _In_ size_t pNumberOfTasks);

    /**
     * Wait for all threads to execute their jobs.
     * @param pThreadPool The thread pools
     */
    W_SYSTEM_EXPORT
        void w_thread_pool_wait_all(_Inout_ w_thread_pool pThreadPool);

    /**
    * Destroy the thread pool and stop all the threads
    * @param pThreadPool the thread pool
    * @returns APR_BADARG on bad functions arguments
    * @return APR_SUCCESS if all threads are stopped
    */
    W_SYSTEM_EXPORT
        W_RESULT w_thread_pool_fini(_Inout_ w_thread_pool* pThreadPool);

#ifdef __cplusplus
}
#endif

