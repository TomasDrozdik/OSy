// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <proc/mutex.h>

typedef struct list_item {
    link_t link;
    mutex_t* mutex;
    list_t queue;
    thread_t* owner;
} mutex_list_item;

list_t mutex_list;

//Find mutex in our list
mutex_list_item mutex_list_find(mutex_t* mutex) {
    list_foreach(mutex_list, mutex_list_item, link, item) {
        if (item.mutex == mutex) {
            return item;
        }
    }
    panic("Mutex already destroyed.");
}

/** Initializes given mutex.
 *
 * @param mutex Mutex to initialize.
 * @return Error code.
 * @retval EOK Mutex was successfully initialized.
 */
errno_t mutex_init(mutex_t* mutex) {
    mutex->locked = false;
    mutex_list_item new_item;
    new_item.mutex = mutex;
    link_init(&new_item.link);
    new_item.owner = thread_get_current();
    list_init(&new_item.queue);
    list_append(&mutex_list, &new_item.link);
	//TODO initialize list
    return EOK;
}

/** Destroys given mutex.
 *
 * The function must panic if the mutex is still locked when being destroyed.
 *
 * @param mutex Mutex to destroy.
 */
void mutex_destroy(mutex_t* mutex) {
    mutex_list_item item = mutex_list_find(mutex);
    panic_if(mutex->locked, "Mutex locked when thread tried to destroy!");
    list_remove(&item.link);
    item = NULL;

    mutex = NULL;
}

/** Locks the mutex.
 *
 * Note that when this function returns, the mutex must be locked.
 *
 * @param mutex Mutex to be locked.
 */
void mutex_lock(mutex_t* mutex) {
    mutex_list_item item = mutex_list_find(mutex);
    if (mutex_trylock(mutex) == EBUSY) {
        thread_t* current = thread_get_current();
        list_remove(&current->link);
        current->state = WAITING;
        list_append(&item.queue, current);
        thread_yield();
    }
    mutex->locked = true;
    item.owner = thread_get_current();
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
    mutex_list_item item = mutex_list_find(mutex);
    panic_if(item->owner != thread_get_current(), "Different thread trying to unlock.");
    mutex->locked = false;
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
    if (mutex->locked) {
        return EBUSY;
    } else {
        mutex->locked = true;
        return EOK;
    }
}
