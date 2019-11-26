// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug.h>
#include <proc/scheduler.h>
#include <adt/list.h>
#include <mm/heap.h>

#include <lib/print.h>

#define INDEX_OF(ITEMPTR, ARRAYPTR) \
    ((size_t)(((uintptr_t)ITEMPTR - (uintptr_t)ARRAYPTR) / sizeof(*ARRAYPTR)))

#define THREAD_POOL_SIZE 100

typedef struct queue_item {
    thread_t* thread;
    link_t link;
    thread_state_t state;
} queue_item_t;

static size_t queue_size;
static queue_item_t* current_thread;
static queue_item_t* schedule_pool;

static list_t ready_thread_queue;
static list_t suspended_thread_queue;

static size_t free_indices_stack_top;
static size_t* free_indices_stack;  // Grows downwards

/** Scheduling stategy.
 *
 * Puts item in list as the last one i.e. before current thread.
 */
static inline void schedule(queue_item_t* item);

/** Increases the size of preallocated space according to external size of the
 *  thread pool.
 */
static void resize(void);

/** Initialize support for scheduling.
 *
 * Called once at system boot.
 */
void scheduler_init(void) {
    queue_size = THREAD_POOL_SIZE;
    schedule_pool = (queue_item_t*)
            kmalloc(sizeof(queue_item_t) * queue_size);

    free_indices_stack = (size_t*)
            kmalloc(sizeof(size_t) * queue_size);

    panic_if(!schedule_pool || !free_indices_stack,
            "scheduler_init: Not enough memory.");

    for (size_t i = 0; i < queue_size; ++i) {
        free_indices_stack[i] = i;
    }
    // Stack grows downwards i.e. full stack starts at 0.
    free_indices_stack_top = 0;

    list_init(&ready_thread_queue);
    list_init(&suspended_thread_queue);

    // Since no thread is running set this to NULL.
    current_thread = NULL;
}

/** Marks given thread as ready to be executed.
 *
 * It is expected that this thread would be added at the end
 * of the queue to run in round-robin fashion.
 *
 * @param thread Thread to make runnable.
 */
void scheduler_add_ready_thread(thread_t* thread) {
    dprintk("\n");

    // Check if there is a free space in preallocated array of queue_item_t.
    if (free_indices_stack_top == queue_size) {
        resize();
    }

    queue_item_t* new =
            &schedule_pool[free_indices_stack[free_indices_stack_top++]];

    new->thread = thread;

    // If no thread has beed scheduled yet then add this new one as a first.
    if (current_thread == NULL) {
        list_append(&ready_thread_queue, &new->link);
        current_thread = new;
    } else {
        // Otherwise just schedule the thread using specific scheduling
        // stretegy.
        schedule(new);
    }
}

/** Removes given thread from scheduling.
 *
 * Expected to be called when thread is suspended, terminates execution
 * etc.
 *
 * @param thread Thread to remove from the queue.
 */
void scheduler_remove_thread(thread_t* thread) {
    dprintk("\n");

    list_foreach(ready_thread_queue, queue_item_t, link, queue_item) {
        if (queue_item->thread == thread) {
            list_remove(&queue_item->link);
            free_indices_stack[++free_indices_stack_top] =
                    INDEX_OF(queue_item, schedule_pool);
            return;
        }
    }
}

void scheduler_remove_current_thread() {
    dprintk("\n");

    list_remove(&current_thread->link);
    free_indices_stack[++free_indices_stack_top] =
            INDEX_OF(current_thread, schedule_pool);
}

/** Suspends given thread in scheduling.
 *
 * Expected to be called when thread is suspended.
 *
 * @param thread Thread to remove from the queue.
 */
void scheduler_suspend_thread(thread_t* thread) {
    // TODO:
    panic();
}

void scheduler_suspend_current_thread() {
    dprintk("\n");

    // Remove this thread from the list of ready threads.
    list_remove(&current_thread->link);

    // Add it to queue of suspended threads.
    current_thread->state = SUSPENDED;
    list_append(&suspended_thread_queue, &current_thread->link);

    scheduler_schedule_next();
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
errno_t scheduler_wakeup_thread(thread_t *id) {
    dprintk("\n");

    if (id->state == FINISHED) {
        return EEXITED;
    }
    list_foreach(suspended_thread_queue, queue_item_t, link, suspended_thread) {
        if (suspended_thread->thread == id) {
            list_remove(&suspended_thread->link);

            suspended_thread->state = READY;
            schedule(suspended_thread);
            return EOK;
        }
    }
    return EINVAL;
}

/** Switch to next thread in the queue. */
void scheduler_schedule_next(void) {
    dprintk("\n");

    if (current_thread == NULL) {
        panic("scheduler_schedule_next: Current thread is NULL.");
    }

    link_t* next_link = current_thread->link.next;
    if (!valid_link(ready_thread_queue, next_link)) {
        next_link = next_link->next;
        assert(valid_link(ready_thread_queue, next_link));
    }

    current_thread = list_item(current_thread->link.next, queue_item_t, link);

    thread_switch_to(current_thread->thread);
}

thread_t* scheduler_get_running_thread() {
    dprintk("\n");

    return (current_thread == NULL) ? NULL : current_thread->thread;
}

static inline void schedule(queue_item_t* item) {
    dprintk("Thread %s schedules in %pL\n", item->thread->name,
            &ready_thread_queue);

    list_add(current_thread->link.prev, &item->link);
}

/** Double the size of the thread_queue, free_indices_stack and correct pointers
 *  in link.
 *
 *  This happens when there are no free indices in thread_queue.
 *
 *  First thread_queue size is doubled and values are copied to new array.
 *
 *  Then free_indices_stack is also doubled and half of it is filled with new
 *  free indices i.e. upper half of thread_queue indices.
 *
 *  Finally since all queue_item structures have been reallocated we have to
 *  reinitialize the array and whole first half thread_queue to it.
 *
 *  Furthermore all globals corresponding to mentioned data structures need
 *  correction.
 */
static void resize() {
    // TODO:
    panic();
}
