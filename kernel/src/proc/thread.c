// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/list.h>
#include <debug/code.h>
#include <exc.h>
#include <lib/print.h>
#include <mm/frame.h>
#include <mm/heap.h>
#include <proc/context.h>
#include <proc/scheduler.h>
#include <proc/thread.h>
#include <utils.h>

/** Calculates address of initial stack top of given thread.
 *
 * @param threadptr Pointer to thread.
 * @return Address of stack top of given thread in unative_t.
 */
#define THREAD_INITIAL_STACK_TOP(threadptr) \
    ((unative_t)((threadptr)->stack + THREAD_STACK_SIZE))

/** Calculates pointer to initial context of given thread.
 *
 * This context is stored at the top of the stack.
 *
 * @param THREADPTR Pointer to thread.
 * @returns Pointer to initial context_t.
 */
#define THREAD_INITIAL_CONTEXT(threadptr) \
    ((context_t*)(THREAD_INITIAL_STACK_TOP(threadptr) - sizeof(context_t)))

/** Points to currently running thread_t.
 *
 * WARNING: this shoud be changed ONLY immediatelly followed by context switch
 *          i.e. inside of thread_switch_to function.
 */
static thread_t* running_thread;

/** Wraps the thread_entry_function so that it always calls finish.
 */
static void thread_entry_func_wrapper(void);

/** Free address space assigned to the thread also invalidate TLB for it's ASID.
 *
 * @param thread Thread to free as form.
 */
static inline void thread_free_as(thread_t *thread);

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
 * This thread will use the same address space as the current one.
 *
 * @param thread_out Where to place the initialized thread_t structure.
 * @param entry Thread entry function.
 * @param data Data for the entry function.
 * @param flags Flags: 0 -> KENEL thread
 *                     1 -> USERSPACE thread
 * @param name Thread name (for debugging purposes).
 * @return Error code.
 * @retval EOK Thread was created and started (added to ready queue).
 * @retval ENOMEM Not enough memory to complete the operation.
 * @retval INVAL Invalid flags (unused).
 */
errno_t thread_create(thread_t** thread_out, thread_entry_func_t entry, void* data, unsigned int flags, const char* name) {
    bool enable = interrupts_disable();  // Enter critical section.

    // Allocate enought memory thread_t structure.
    thread_t* thread = (thread_t*)kmalloc(sizeof(thread_t));
    if (thread == NULL) {
        interrupts_restore(enable);
        return ENOMEM;
    }

    // Allocate memory for thread_stack
    errno_t err = kframe_alloc(
            round_up(THREAD_STACK_SIZE, PAGE_SIZE) / PAGE_SIZE,
            &thread->stack);
    if (err != EOK) {
        // First cleanup.
        kfree(thread);
        assert(err == ENOMEM);
        return err;
    }

    // Set up thread_t structure.
    thread->type = flags ? USERSPACE : KERNEL;
    thread->state = READY;
    strncpy((char*)thread->name, name, THREAD_NAME_MAX_LENGTH);

    thread->context = THREAD_INITIAL_CONTEXT(thread);
    // TODO: check if there is need to init this thread->stack =

    thread->entry_func = entry;
    thread->data = data;

    // So far thread belongs to no process, this can be changed later on with
    // thread_assign_to_process().
    thread->process = NULL;

    // Set up context
    thread->context->sp = THREAD_INITIAL_STACK_TOP(thread);
    thread->context->ra = (unative_t)&thread_entry_func_wrapper;
    thread->context->status = 0xff01;

    // Inherit address space from currently running thread.
    thread->as = (running_thread) ? running_thread->as : NULL;
    if (thread->as) {
        ++(thread->as->reference_counter);
    }

    scheduler_add_ready_thread(thread);
    *thread_out = thread;

    interrupts_restore(enable);
    return EOK;
}

/** Create a new thread with new address space.
 *
 * The thread is automatically placed into the queue of ready threads.
 *
 * This function allocates space for both stack and the thread_t structure
 * (hence the double <code>**</code> in <code>thread_out</code>.
 *
 * @param thread_out Where to place the initialized thread_t structure.
 * @param entry Thread entry function.
 * @param data Data for the entry function.
 * @param flags Flags: 0 -> KENEL thread
 *                     1 -> USERSPACE thread
 * @param name Thread name (for debugging purposes).
 * @param as_size Address space size, aligned at page size (0 is correct though not very useful).
 * @return Error code.
 * @retval EOK Thread was created and started (added to ready queue).
 * @retval ENOMEM Not enough memory to complete the operation.
 * @retval INVAL Invalid flags (unused).
 */
errno_t thread_create_new_as(thread_t** thread_out, thread_entry_func_t entry,
        void* data, unsigned int flags, const char* name, size_t as_size) {
    bool enable = interrupts_disable();

    // First we create a thread inheriting previous address space
    errno_t err = thread_create(thread_out, entry, data, flags, name);
    if (err != EOK) {
        interrupts_restore(enable);
        return err;
    }
    if ((*thread_out)->as) {
        // If there is an inherited AS than reference counter was increased so
        // since we are creating a new AS we decrease the counter.
        --((*thread_out)->as->reference_counter);
    }
    (*thread_out)->as = as_create(as_size, 0);
    if ((*thread_out)->as == NULL) {
        // First cleanup.
        kfree(*thread_out);
        interrupts_restore(enable);
        return ENOMEM;
    }
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
    bool enable = interrupts_disable();

    scheduler_suspend_thread(running_thread);
    scheduler_schedule_next();

    interrupts_restore(enable);
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
    interrupts_disable();

    running_thread->state = FINISHED;
    running_thread->retval = retval;

    assert(running_thread == scheduler_get_scheduled_thread());
    scheduler_remove_thread(running_thread);
    scheduler_schedule_next();

    // Noreturn function
    assert(0 && "Reached noreturn path.");
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
 * @retval EKILLED Thread was killed.
 * @retval EINVAL Invalid thread.
 */
errno_t thread_join(thread_t* thread, void** retval) {
    bool enable = interrupts_disable();

    if (thread == NULL) {
        interrupts_restore(enable);
        return EINVAL;
    }

    while (!(thread->state == FINISHED || thread->state == KILLED)) {
        thread_yield();
    }
    if (thread->state == KILLED) {
        return EKILLED;
    }

    // Release the address space since we will no longer access this thread's
    // assigned space.
    thread_free_as(thread);

    if (retval) {
        *retval = thread->retval;
    }

    interrupts_restore(enable);
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
        stack_top_old = (void**)&running_thread->context;
    }

    void** stack_top_new = (void**)&thread->context;
    running_thread = scheduler_get_scheduled_thread();
    cpu_switch_context(stack_top_old, stack_top_new, running_thread->as->asid);

    interrupts_restore(enable);
}

/** Get address space of given thread. */
as_t* thread_get_as(thread_t* thread) {
    return thread->as;
}

/** Kills given thread.
 *
 * Note that this function shall work for any existing thread, including
 * currently running one.
 *
 * Joining a killed thread results in EKILLED return value from thread_join.
 *
 * @param thread Thread to kill.
 * @return Error code.
 * @retval EOK Thread was killed.
 * @retval EINVAL Invalid thread.
 * @retval EEXITED Thread already finished its execution.
 */
errno_t thread_kill(thread_t* thread) {
    bool enable = interrupts_disable();

    thread->state = KILLED;
    // Release the address space since we will no longer access this thread's
    // assigned space.
    // thread_free_as(thread);

    scheduler_remove_thread(thread);
    if (thread == running_thread) {
        scheduler_schedule_next();
        // Noreturn path
        assert(0 && "Reached noreturn path.");
        while (1)
            ;
    }

    interrupts_restore(enable);
    return EOK;
}

void thread_assign_to_process(process_t* process) {
    // Critical section -> global runnging thread
    bool enable = interrupts_disable();
    panic_if(running_thread->type != USERSPACE,
            "Assigning process to a non userspace thread.\n");
    panic_if(running_thread->process != NULL,
            "Assigning process to thread which already has a thread assigned.\n");
    running_thread->process = process;
    interrupts_restore(enable);  // End of critical section.
}

static void thread_entry_func_wrapper() {
    panic_if(running_thread == NULL,
            "thread_entry_func_wrapper: running_thread == NULL");
    thread_finish(running_thread->entry_func(running_thread->data));
}

static inline void thread_free_as(thread_t *thread) {
    if (thread->as) {
        as_destroy(thread->as);
    }
}

