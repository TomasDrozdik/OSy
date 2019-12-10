// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <proc/mutex.h>
#include <mm/heap.h>
#include <exc.h>

typedef struct list_item {
    link_t link;
    mutex_t* mutex;
    list_t queue;
    thread_t* owner;
} mutex_list_item;

bool mutex_first = true;

//List of mutexes.
static list_t mutex_list;

/** Wakes up next thread in queue of given mutex.
 *
 * @param mutex Item with apropriate mutex.
 * 
 */
void mutex_wake_up_next(mutex_list_item* item);

/** Finds given mutex.
 *
 * @param mutex Mutex to find.
 * @return Item containing the mutex.
 */
mutex_list_item* mutex_list_find(mutex_t* mutex);

/** Initializes given mutex.
 *
 * @param mutex Mutex to initialize.
 * @return Error code.
 * @retval EOK Mutex was successfully initialized.
 */
errno_t mutex_init(mutex_t* mutex) {
    bool enable = interrupts_disable();

    if (mutex_first) {
        list_init(&mutex_list);
        mutex_first = false;
	}
    mutex->locked = false;
    mutex_list_item* new_item = (mutex_list_item*)kmalloc(sizeof(mutex_list_item));
    new_item->mutex = mutex;
    link_init(&new_item->link);
    list_init(&new_item->queue);
    list_append(&mutex_list, &new_item->link);

	interrupts_restore(enable);
	
    return EOK;
}

void mutex_wake_up_next(mutex_list_item* item) {
    bool enable = interrupts_disable();

    thread_t* next_thread = list_item(list_pop(&item->queue), thread_t, link);
	next_thread->state = READY;
	scheduler_add_ready_thread(next_thread);

	interrupts_restore(enable);
}

mutex_list_item* mutex_list_find(mutex_t* mutex) {
    list_foreach(mutex_list, mutex_list_item, link, item) {
        if (item->mutex == mutex) {
            return item;
        }
    }
    panic("Mutex already destroyed.");
    return NULL;
}

/** Destroys given mutex.
 *
 * The function must panic if the mutex is still locked when being destroyed.
 *
 * @param mutex Mutex to destroy.
 */
void mutex_destroy(mutex_t* mutex) {
    bool enable = interrupts_disable();

    mutex_list_item* item = mutex_list_find(mutex);
    panic_if(mutex->locked==true, "Mutex still locked when a thread tried to destroy it!");
    list_remove(&item->link);
    kfree(item);

	interrupts_restore(enable);

    mutex = NULL;
}

/** Locks the mutex.
 *
 * Note that when this function returns, the mutex must be locked.
 *
 * @param mutex Mutex to be locked.
 */
void mutex_lock(mutex_t* mutex) {
    bool enable = interrupts_disable();

    mutex_list_item* item = mutex_list_find(mutex);
	
    while (mutex_trylock(mutex) == EBUSY) {
        thread_t* current = thread_get_current();
        scheduler_suspend_thread(current);
        current->state = WAITING;
        list_remove(&current->link);
        list_append(&item->queue, &current->link);
        //printk("Waiting!\n");
		interrupts_restore(enable);

        thread_yield();

		enable = interrupts_disable();
    }
    mutex->locked = true;
    item->owner = thread_get_current();
    //printk("successfully locked.\n");

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

    mutex_list_item* item = mutex_list_find(mutex);
    panic_if(item->owner != thread_get_current(), "Different thread trying to unlock mutex.");
    
	mutex->locked = false;
    //printk("Unlocked.\n");
	
    if(list_get_size(&item->queue)!=0){
		mutex_wake_up_next(item);
        //printk("Woke up next\n");
		interrupts_restore(enable);
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
        interrupts_restore(enable);
        return EOK;
    }
}
