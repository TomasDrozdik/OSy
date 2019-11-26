// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug.h>
#include <proc/scheduler.h>
#include <adt/list.h>

#define INDEX_OF(ITEMPTR, ARRAYPTR) \
    ((size_t)(((uintptr_t)ITEMPTR - (uintptr_t)ARRAYPTR) / sizeof(*ARRAYPTR)))

#define THREAD_POOL_SIZE 100

static size_t queue_size;
static queue_item_t* current_thread;
static queue_item_t* schedule_pool;

static list_t ready_thread_queue;
static list_t sleeping_thread_queue;

static size_t free_indices_stack_top;
static size_t* free_indices_stack;  // Grows downwards

typedef enum thread_state {
    Ready,
    Suspended,
} thread_state_t;

typedef struct queue_item {
    thread_t* thread;
    link_t link;
    thread_state_t state;
} queue_item_t;


/** Increases the size of preallocated space according to external size of the
 *  thread pool.
 */
static void resize();

/** Initialize support for scheduling.
 *
 * Called once at system boot.
 */
void scheduler_init(void) {
    queue_size = THREAD_POOL_SIZE;
    schedule_pool = (typeof(schedule_pool))
            kmalloc(sizeof(queue_item_t) * queue_size);

    free_indices_stack = (size_t*)
            kmalloc(sizeof(size_t) * queue_size);
    for (size_t i = 0; i < queue_size; ++i) {
        free_indices_stack[i] = i;
    }
    // Stack grows downwards i.e. full stack starts at 0.
    free_indices_stack_top = 0;

    list_init(&ready_thread_queue);

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
    // Check if there is a free space in preallocated array of queue_item_t.
    if (free_indices_stack_top == queue_size) {
        resize();
    }

    queue_item_t* new =
            schedule_pool[free_indices_stack[free_indices_stack_top++]];

    new->thread = *thread;
    list_append(&ready_thread_queue, &new->link);

    // If no thread has beed scheduled yet then add this newly added one as
    // a first one. Otherwise switching is managed with
    // scheduler_schedule_next().
    if (current_thread == NULL) {
        current_thread = new;
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
}

void scheduler_suspend_current_thread() {
    // TODO:
}

/** Switch to next thread in the queue. */
void scheduler_schedule_next(void) {
    if (current_thread == NULL) {
        return;
    }

    link_t* next_link = current_thread->link.next;
    if (!valid_link(ready_thread_queue, next_link)) {
        next_link = next_link->next;
        assert(valid_link(next_link));
    }

    current_thread = list_item(current_thread->link.next, queue_item_t, link);

    // TODO: switch context
}

inline thread_t* scheduler_get_running_thread() {
    return (current_thread == NULL) ? NULL : current_thread->thread;
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
