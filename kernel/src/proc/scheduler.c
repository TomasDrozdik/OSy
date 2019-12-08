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
} queue_item_t;

static size_t queue_size;
static queue_item_t* current_item;
static queue_item_t* schedule_pool;

static list_t ready_thread_queue;
static list_t suspended_thread_queue;

static size_t free_indices_stack_top;
static size_t* free_indices_stack;  // Grows downwards

static bool changed_current_item;

/** Scheduling stategy.
 *
 * Puts item in list as the last one i.e. before current thread.
 */
static inline void schedule(queue_item_t* item);

static inline void pick_next_current_item(void);

/** Increases the size of preallocated space according to external size of the
 *  thread pool.
 */
static void resize(void);

//static void debug_print_list() {
//    dprintk("\nScheduler state: %pL\n", &ready_thread_queue);
//    list_foreach(ready_thread_queue, queue_item_t, link, queue_item) {
//        printk("\titem[%p -> %p] %pT\n", &queue_item->link, queue_item->link.next, queue_item->thread);
//    }
//}

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
    current_item = NULL;

    changed_current_item = false;
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
    if (free_indices_stack_top > queue_size) {
        resize();
    }

    queue_item_t* new =
            &schedule_pool[free_indices_stack[free_indices_stack_top++]];

    new->thread = thread;

    if (current_item == NULL) {
//        dprintk("Thread %s schedules in %pL\n", new->thread->name,
//                &ready_thread_queue);
        list_append(&ready_thread_queue, &new->link);
    } else {
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
            if (queue_item->thread == current_item->thread) {
                scheduler_remove_current_thread();
            } else {
                list_remove(&queue_item->link);
                free_indices_stack[++free_indices_stack_top] =
                        INDEX_OF(queue_item, schedule_pool);
            }
            return;
        }
    }
}

void scheduler_remove_current_thread() {
    dprintk("\n");

    queue_item_t* item_to_remove = current_item;

    // Select new current_item, set flag for it and check that this is NOT the
    // last thread.
    pick_next_current_item();
    changed_current_item = true;
    panic_if(current_item == item_to_remove, "Removing last running thread.");

    // Unlink previous item.
    list_remove(&item_to_remove->link);

    // Make space of item_to_remove available.
    free_indices_stack[++free_indices_stack_top] =
            INDEX_OF(item_to_remove, schedule_pool);

}

/** Suspends given thread in scheduling.
 *
 * Expected to be called when thread is suspended.
 *
 * @param thread Thread to remove from the queue.
 */
void scheduler_suspend_thread(thread_t* thread) {
    panic_if(current_item->thread->state != READY,
            "Suspending thread which is not READY.");


    list_foreach(ready_thread_queue, queue_item_t, link, queue_item) {
        if (queue_item->thread == thread) {
            if (queue_item->thread == current_item->thread) {
                scheduler_suspend_current_thread();
            } else {
                thread->state = SUSPENDED;
                list_remove(&queue_item->link);
                list_append(&suspended_thread_queue, &queue_item->link);
            }
            return;
        }
    }
}

void scheduler_suspend_current_thread() {
    dprintk("\n");

    panic_if(current_item->thread->state != READY,
            "Suspending thread which is not READY.");

    queue_item_t* item_to_suspend = current_item;

    // Pick next current_item, note that fact and check if it was the last one.
    pick_next_current_item();
    changed_current_item = true;
    panic_if(current_item == item_to_suspend,
            "Suspending last running thread.");

    // Remove this thread from the list of ready threads.
    list_remove(&item_to_suspend->link);

    // Add it to queue of suspended threads.
    item_to_suspend->thread->state = SUSPENDED;
    list_append(&suspended_thread_queue, &item_to_suspend->link);
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
    dprintk("Waking up %pT\n", id);

    if (id->state == FINISHED) {
        return EEXITED;
    } else if (id->state == READY) {
        return EOK;
    }
    list_foreach(suspended_thread_queue, queue_item_t, link, suspended_item) {
        if (suspended_item->thread == id) {
            list_remove(&suspended_item->link);

            suspended_item->thread->state = READY;
            schedule(suspended_item);
            return EOK;
        }
    }
    return EINVAL;
}

/** Switch to next thread in the queue. */
void scheduler_schedule_next(void) {
//    dprintk("Schedule next from %pL\n", &ready_thread_queue);

    if (current_item == NULL) {  // Very first run of this function
        current_item = list_item(ready_thread_queue.head.next, queue_item_t, link);
    } else if (!changed_current_item) {
        pick_next_current_item();
    } else {
        changed_current_item = false;
    }

    dprintk("Scheduled item: %p, thread_name: %s\n",
            &current_item->link, current_item->thread->name);

    assert(current_item->thread->state == READY);
    thread_switch_to(current_item->thread);
}

thread_t* scheduler_get_current_scheduled_thread(void) {
    return (current_item == NULL) ? NULL : current_item->thread;
}

static inline void schedule(queue_item_t* item) {
//    dprintk("Scheduling thread %s in %pL\n", item->thread->name,
//            &ready_thread_queue);

    assert(current_item != NULL);
    list_add(current_item->link.prev, &item->link);

//    dprintk("Thread %s is scheduled in %pL\n", item->thread->name,
//            &ready_thread_queue);
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
    panic("resize\n");
}

static inline void pick_next_current_item() {
    panic_if(list_is_empty(&ready_thread_queue),
            "No active threads in scheduler.");

    link_t* next_link = current_item->link.next;
    if (!valid_link(ready_thread_queue, next_link)) {
        // next_link points to head of the list so picking another next link
        // would point to valid item since the list is not empty, checked above.
        next_link = next_link->next;
        assert(valid_link(ready_thread_queue, next_link));
    }

    current_item = list_container_of(next_link, queue_item_t, link);

    if (current_item->thread->state != READY) {
        dprintk("unREADY in ready_queue %pT\n", current_item->thread);
        assert(current_item->thread->state == READY);
    }
}
