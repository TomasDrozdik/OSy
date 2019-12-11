// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/list.h>
#include <debug.h>
#include <mm/heap.h>
#include <proc/scheduler.h>
#include <drivers/timer.h>
#include <lib/print.h>
#include <exc.h>

/** List of ready threads. */
static list_t ready_thread_queue;

/** List of suspended threads. */
static list_t suspended_thread_queue;

/** Pointer to currently scheduled thread.
 *  WARNING: Not always is it the currently running thread see thread.c.
*/
static thread_t* scheduled_thread;

/** Flag signaling that some outside funciton apart from scheduler_schedule_next
 *  called pick_next_scheduled_thread and so following call to this funciton
 *  would not pick next one.
 *
 *  This is done to preserve proper Round Robin fairness.
 */
static bool changed_scheduled_thread;

/** Schedules thread to ready_thread_queue.
 *
 * Strategy: schedules just before current scheduled_thread i.e.
 *           Round Robin strategy.
 *
 * Puts item in list as the last one i.e. before current thread.
 * Or in case of first thread just appends to ready_thread_queue.
 * WARNING: thread->state must be READY
 *
 * @param thread Thread to schedule in ready_thread_queue.
 */
static inline void schedule(thread_t* thread);

/** Sets next scheduled_thread in round robin fashion.
 *
 * Checks whether the list is empty in which case it panics.
 */
static inline void pick_next_scheduled_thread(void);

/** Initialize support for scheduling.
 *
 * Called once at system boot.
 */
void scheduler_init(void) {
    bool enable = interrupts_disable();

    list_init(&ready_thread_queue);
    list_init(&suspended_thread_queue);
    scheduled_thread = NULL;
    changed_scheduled_thread = false;

	interrupts_restore(enable);
}

/** Marks given thread as ready to be executed.
 *
 * It is expected that this thread would be added at the end
 * of the queue to run in round-robin fashion.
 *
 * @param thread Thread to make runnable.
 */
void scheduler_add_ready_thread(thread_t* thread) {
    assert(!link_is_connected(&thread->link));
    schedule(thread);
}

/** Removes given thread from scheduling.
 *
 * Resolves case when removed thread is current scheduled_thread in which case
 * it picks next scheduled thread setting changed_scheduled_thread flag.
 *
 * WARNING: This call should be followed by scheduler_schedule_next()
 *
 * @param thread Thread to remove from the queue.
 */
void scheduler_remove_thread(thread_t* thread) {
    bool enable = interrupts_disable();

    assert(thread->state == FINISHED);
    if (thread == scheduled_thread) {
        pick_next_scheduled_thread();
        panic_if(scheduled_thread == thread, "Removing last thread.");
        changed_scheduled_thread = true;
    }
    list_remove(&thread->link);

	interrupts_restore(enable);
}

/** Suspends given thread in scheduling.
 *
 * Resolves case when removed thread is current scheduled_thread in which case
 * it picks next scheduled thread setting changed_scheduled_thread flag.
 *
 * WARNING: This call should be followed by scheduler_schedule_next()
 *
 * @param thread Thread to remove from the queue.
 */
void scheduler_suspend_thread(thread_t* thread) {
    bool enable = interrupts_disable();

    panic_if(scheduled_thread->state != READY,
            "Suspending thread which is not READY.");
    thread->state = SUSPENDED;
    if (thread == scheduled_thread) {
        pick_next_scheduled_thread();
        panic_if(scheduled_thread == thread, "Suspending last thread.");
        changed_scheduled_thread = true;
    }
    list_remove(&thread->link);
    list_append(&suspended_thread_queue, &thread->link);

	interrupts_restore(enable);
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
 * @retval EEXITED Thread already finished its execution.
 * TODO: @retval EINVAL Invalid thread.
 */
errno_t scheduler_wakeup_thread(thread_t* thread) {
    bool enable = interrupts_disable();

    if (thread->state == FINISHED) {
        interrupts_restore(enable);
        return EEXITED;
    } else if (thread->state == READY) {
        interrupts_restore(enable);
        return EOK;
    }

    assert(thread->state == SUSPENDED);

    list_remove(&thread->link);
    thread->state = READY;
    schedule(thread);

    interrupts_restore(enable);
    return EOK;
}

/** Switch to next thread in the queue.
 * By calling context switch managed by thread_switch_to function.
 *
 * In case flag changed_scheduled_thread we do not pick next one just schedule
 * the current one.
*/
void scheduler_schedule_next(void) {
    bool enable = interrupts_disable();

    if (scheduled_thread == NULL) { // Very first run of this function
        scheduled_thread = list_item(ready_thread_queue.head.next, thread_t, link);
    } else if (!changed_scheduled_thread) {
        pick_next_scheduled_thread();
    } else {
        changed_scheduled_thread = false;
    }

    assert(scheduled_thread->state == READY);
    thread_switch_to(scheduled_thread);

	interrupts_restore(enable);
}

/** Returns the thread the scheduler has currently scheduled.
 *
 * WARNING: may not be currently running thread.
 */
thread_t* scheduler_get_scheduled_thread(void) {
    return scheduled_thread;
}

static inline void schedule(thread_t* thread) {
    bool enable = interrupts_disable();

    assert(thread->state == READY);
    if (scheduled_thread != NULL) {
        list_add(scheduled_thread->link.prev, &thread->link);
    } else { // Init thread
        list_append(&ready_thread_queue, &thread->link);

		//Set interrupt to 1500 cycles(subject to change)
        timer_interrupt_after(CYCLES);
    }

	interrupts_restore(enable);
}

static inline void pick_next_scheduled_thread() {
    bool enable = interrupts_disable();

    panic_if(list_is_empty(&ready_thread_queue),
            "No active threads in scheduler.");

    link_t* next_link = scheduled_thread->link.next;
    if (!valid_link(ready_thread_queue, next_link)) {
        // next_link points to head of the list so picking another next link
        // would point to valid item since the list is not empty, checked above.
        next_link = next_link->next;
        assert(valid_link(ready_thread_queue, next_link));
    }

    scheduled_thread = list_container_of(next_link, thread_t, link);
    assert(scheduled_thread->state == READY);

	interrupts_restore(enable);
}
