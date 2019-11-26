// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <lib/print.h>
#include <proc/context.h>
#include <proc/scheduler.h>
#include <proc/thread.h>
#include <adt/list.h>
#include <mm/heap.h>

/** Wraps the thread_entry_function so that it always calls finish.
 */
static void thread_entry_func_wrapper(void);

/** Initialize support for threading.
 *
 * Called once at system boot.
 */
void threads_init(void) {
}

/** Create a new thread.
 *
 * The thread is automatically placed into the queue of ready threads.
 *
 * This function allocates space for both stack and the thread_t structure
 * (hence the double <code>**</code> in <code>thread_out</code>.
 *
 * WARNIGN: Caller is responsible for freeing this pointer after the thread
 *          finishes and caller has dealt with return value if need be.
 *
 * @param thread_out Where to place the initialized thread_t structure.
 * @param entry Thread entry function.
 * @param data Data for the entry function.
 * @param flags Flags (unused).
 * @param name Thread name (for debugging purposes).
 * @return Error code.
 * @retval EOK Thread was created and started (added to ready queue).
 * @retval ENOMEM Not enough memory to complete the operation.
 * @retval INVAL Invalid flags (unused).
 */
errno_t thread_create(thread_t** thread_out, thread_entry_func_t entry, void* data, unsigned int flags, const char* name) {
    // Allocate enought memory for stack and thread_t structure.
    // No heap is available.
    thread_t* thread = (thread_t*)kmalloc(sizeof(thread_t) + THREAD_STACK_SIZE);
    if (thread == NULL) {
        return ENOMEM;
    }

    // Set up thread_t structure.
    strncpy((char*)thread->name, name, THREAD_NAME_MAX_LENGTH);
    thread->entry_func = entry;
    thread->data = data;
    thread->state = READY;
    thread->stack_top = (unative_t)
            ((uintptr_t)thread + sizeof(thread_t) + THREAD_STACK_SIZE);
    *thread_out = thread;

    // Set up stack
    thread->stack_top -= sizeof(context_t);
    context_t* context = (context_t*)thread->stack_top;
    context->sp = thread->stack_top;
    context->ra = (unative_t)&thread_entry_func_wrapper;
    context->status = 0xff01;

    scheduler_add_ready_thread(*thread_out);
    return EOK;
}

/** Return information about currently executing thread.
 *
 * @retval NULL When no thread was started yet.
 */
thread_t* thread_get_current(void) {
    return scheduler_get_running_thread();
}

/** Yield the processor. */
void thread_yield(void) {
    scheduler_schedule_next();
}

/** Current thread stops execution and is not scheduled until woken up. */
void thread_suspend(void) {
    scheduler_suspend_current_thread();
}

/** Terminate currently running thread.
 *
 * Thread can (normally) terminate in two ways: by returning from the entry
 * function of by calling this function. The parameter to this function then
 * has the same meaning as the return value from the entry function.
 *
 * Note that this function never returns.
 *
 * @param retval Data to return in thread_join.
 */
void thread_finish(void* retval) {
    thread_t* current_thread = thread_get_current();
    current_thread->state = FINISHED;
    current_thread->retval = retval;

    scheduler_remove_current_thread();
    scheduler_schedule_next();

    // Noreturn function
    while (1);
}

/** Tells if thread already called thread_finish() or returned from the entry
 * function.
 *
 * @param thread Thread in question.
 */
bool thread_has_finished(thread_t* thread) {
    return thread->state == FINISHED;
}

/** Wakes-up existing thread.
 *
 * Note that waking-up a running (or ready) thread has no effect (i.e. the
 * function shall not count wake-ups and suspends).
 *
 * Note that waking-up a thread does not mean that it will immediatelly start
 * executing.
 *
 * @param thread Thread to wake-up.
 * @return Error code.
 * @retval EOK Thread was woken-up (or was already ready/running).
 * @retval EINVAL Invalid thread.
 * @retval EEXITED Thread already finished its execution.
 */
errno_t thread_wakeup(thread_t* thread) {
    return scheduler_wakeup_thread(thread);
}

/** Joins another thread (waits for it to terminate.
 *
 * Note that <code>retval</code> could be <code>NULL</code> if the caller
 * is not interested in the returned value.
 *
 * @param thread Thread to wait for.
 * @param retval Where to place the value returned from thread_finish.
 * @return Error code.
 * @retval EOK Thread was joined.
 * @retval EBUSY Some other thread is already joining this one.
 * @retval EINVAL Invalid thread.
 */
errno_t thread_join(thread_t* thread, void** retval) {
    if (thread == NULL) {
        return EINVAL;
    }
    while (thread->state != FINISHED) {
        thread_yield();
    }
    *retval = thread->retval;
    return EOK;
}

/** Switch CPU context to a different thread.
 *
 * Note that this function must work even if there is no current thread
 * (i.e. for the very first context switch in the system).
 *
 * @param thread Thread to switch to.
 */
void thread_switch_to(thread_t* thread) {
    void* stack_top_old = (void*)thread_get_current()->stack_top;
    void* stack_top_new = (void*)thread->stack_top;
    cpu_switch_context(&stack_top_old, &stack_top_new, 1);
}

static void thread_entry_func_wrapper()
{
    thread_t* current_thread = thread_get_current();
    thread_finish(current_thread->entry_func(current_thread->data));
}