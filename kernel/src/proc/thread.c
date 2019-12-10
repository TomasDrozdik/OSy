// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/list.h>
#include <debug/code.h>
#include <lib/print.h>
#include <mm/heap.h>
#include <proc/context.h>
#include <proc/scheduler.h>
#include <proc/thread.h>
#include <exc.h>

/** Calculates address of initial stack top of given thread.
 *
 * @param THREADPTR Pointer to thread.
 * @return Address of stack top of given thread in unative_t.
 */
#define THREAD_INITIAL_STACK_TOP(THREADPTR) \
    ((unative_t)((uintptr_t)THREADPTR + sizeof(thread_t) + THREAD_STACK_SIZE))

/** Calculates pointer to initial context of given thread.
 *
 * This context is stored at the top of the stack.
 *
 * @param THREADPTR Pointer to thread.
 * @returns Pointer to initial context_t.
 */
#define THREAD_INITIAL_CONTEXT(THREADPTR) \
    ((context_t*)(THREAD_INITIAL_STACK_TOP(THREADPTR) - sizeof(context_t)))

/** Points to currently running thread_t.
 *
 * WARNING: this shoud be changed ONLY immediatelly followed by context switch
 *          i.e. inside of thread_switch_to function.
 */
static thread_t* running_thread;

/** Wraps the thread_entry_function so that it always calls finish.
 */
static void thread_entry_func_wrapper(void);

/** Initialize support for threading.
 *
 * Called once at system boot.
 */
void threads_init(void) {
    running_thread = NULL;
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
    bool enable = interrupts_disable();
	
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
    thread->context = THREAD_INITIAL_CONTEXT(thread);

    // Set up context
    thread->context->sp = THREAD_INITIAL_STACK_TOP(thread);
    thread->context->ra = (unative_t)&thread_entry_func_wrapper;
    thread->context->status = 0xff01;

    *thread_out = thread;
    scheduler_add_ready_thread(*thread_out);

	interrupts_restore(enable);
    return EOK;
}

/** Return information about currently executing thread.
 *
 * @retval NULL When no thread was started yet.
 */
thread_t* thread_get_current(void) {
    return running_thread;
}

/** Yield the processor. */
void thread_yield(void) {
    scheduler_schedule_next();
}

/** Current thread stops execution and is not scheduled until woken up. */
void thread_suspend(void) {
    scheduler_suspend_thread(running_thread);
    scheduler_schedule_next();
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
    bool enable = interrupts_disable();

    running_thread->state = FINISHED;

    running_thread->retval = retval;

    assert(running_thread == scheduler_get_scheduled_thread());
    scheduler_remove_thread(running_thread);

    interrupts_restore(enable);

    scheduler_schedule_next();

    // Noreturn function
    while (1)
        ;
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

    if (retval) {
        *retval = thread->retval;
    }
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
    bool enable = interrupts_disable();

    void** stack_top_old;
    if (running_thread == NULL) {
        stack_top_old = (void**)debug_get_stack_pointer();
    } else {
        stack_top_old = (void**)&thread_get_current()->context;
    }

    void** stack_top_new = (void**)&thread->context;

    running_thread = scheduler_get_scheduled_thread();

    cpu_switch_context(stack_top_old, stack_top_new, 1);

	interrupts_restore(enable);
}

static void thread_entry_func_wrapper() {
    panic_if(running_thread == NULL,
            "thread_entry_func_wrapper: running_thread == NULL");
    thread_finish(running_thread->entry_func(running_thread->data));
}
