// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <exc.h>
#include <mm/heap.h>
#include <proc/mutex.h>

/** Wakes up next thread in queue of given mutex.
 *
 * @param mutex Item with apropriate mutex.
 * 
 */
static void mutex_wake_up_next(mutex_t* mutex) {
    bool enable = interrupts_disable();

    thread_t* next_thread = list_item(list_pop(&mutex->queue), thread_t, link);
    next_thread->state = READY;
    scheduler_add_ready_thread(next_thread);

    interrupts_restore(enable);
}

/** Initializes given mutex.
 *
 * @param mutex Mutex to initialize.
 * @return Error code.
 * @retval EOK Mutex was successfully initialized.
 */
errno_t mutex_init(mutex_t* mutex) {
    bool enable = interrupts_disable();

    mutex->locked = false;
    list_init(&mutex->queue);

    interrupts_restore(enable);
    return EOK;
}

/** Destroys given mutex.
 *
 * The function must panic if the mutex is still locked when being destroyed.
 *
 * @param mutex Mutex to destroy.
 */
void mutex_destroy(mutex_t* mutex) {
    bool enable = interrupts_disable();

    panic_if(mutex->locked == true, "Mutex still locked when a thread tried to destroy it!");

    interrupts_restore(enable);
}

/** Locks the mutex.
 *
 * Note that when this function returns, the mutex must be locked.
 *
 * @param mutex Mutex to be locked.
 */
void mutex_lock(mutex_t* mutex) {
    bool enable = interrupts_disable();

    while (mutex_trylock(mutex) == EBUSY) {
        thread_t* current = thread_get_current();
        scheduler_suspend_thread(current);
        current->state = WAITING;
        list_remove(&current->link);
        list_append(&mutex->queue, &current->link);

        thread_yield();
    }

    interrupts_restore(enable);
}

/** Unlocks the mutex.
 *
 * Note that when this function returns, the mutex might be already locked
 * by a different thread.
 *
 * This function shall panic if the mutex is unlocked by a different thread
 * than the one that locked it.
 *
 * @param mutex Mutex to be unlocked.
 */
void mutex_unlock(mutex_t* mutex) {
    bool enable = interrupts_disable();

    panic_if(mutex->owner != thread_get_current(), "Different thread trying to unlock mutex.");

    mutex->locked = false;

    if (list_get_size(&mutex->queue) != 0) {
        mutex_wake_up_next(mutex);
        thread_yield();
    }

    interrupts_restore(enable);
}

/** Try to lock the mutex without waiting.
 *
 * If the mutex is already locked, do nothing and return EBUSY.
 *
 * @param mutex Mutex to be locked.
 * @return Error code.
 * @retval EOK Mutex was successfully locked.
 * @retval EBUSY Mutex is currently locked by a different thread.
 */
errno_t mutex_trylock(mutex_t* mutex) {
    bool enable = interrupts_disable();

    if (mutex->locked) {
        interrupts_restore(enable);
        return EBUSY;
    } else {
        mutex->locked = true;
        mutex->owner = thread_get_current();
        interrupts_restore(enable);
        return EOK;
    }
}
