// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <proc/sem.h>
#include <mm/heap.h>

bool sem_first = true;

//list of semaphores.
static list_t sem_list;

//Find appropriate semphore in our list
sem_list_item* sem_list_find(sem_t* sem) {
	list_foreach(sem_list, sem_list_item, link, item) {
        if (item->semaphore == sem) {
			return item;
		}
	}
	panic("Semaphore already destroyed.");
    return NULL;
}

//Wake up the next thread in semaphore queue
void sem_wake_up_next(sem_list_item* item) {
    thread_t* next_thread = list_item(list_pop(&item->queue), thread_t, link);
    next_thread->state = READY;
    scheduler_add_ready_thread(next_thread);
}

/** Initializes given semaphore.
 *
 * @param sem Semaphore to initialize.
 * @param value Initial semaphore value (1 effectively creates mutex).
 * @return Error code.
 * @retval EOK Semaphore was successfully initialized.
 */
errno_t sem_init(sem_t* sem, int value) {
    if (sem_first) {
        list_init(&sem_list);
        sem_first = false;
	}
    sem->value = value;
    sem_list_item* new_item = (sem_list_item*)kmalloc(sizeof(sem_list_item));
    new_item->semaphore = sem;
    link_init(&new_item->link);
    list_init(&new_item->queue);
    list_append(&sem_list, &new_item->link);
	
    return EOK;
}

/** Destroys given semaphore.
 *
 * The function must panic if there are threads waiting for this semaphore.
 *
 * @param sem Semaphore to destroy.
 */
void sem_destroy(sem_t* sem) {
    
    sem_list_item* item = sem_list_find(sem);
    panic_if(list_get_size(&item->queue)!=0, "Threads still waiting for semaphore");
    list_remove(&item->link);
    kfree(item);

    sem = NULL;
}

/** Get current value of the semaphore.
 *
 * @param sem Semaphore to query.
 * @return Current semaphore value.
 */
int sem_get_value(sem_t* sem) {
    return sem->value;
}

/** Locks (downs) the semaphore.
 *
 * Decrement the value of this semaphore. If the new value would be negative,
 * block and wait for someone to call sem_post() first.
 *
 * @param sem Semaphore to be locked.
 */
void sem_wait(sem_t* sem) {

    if (sem_trywait(sem)==EBUSY) {
        sem_list_item* item = sem_list_find(sem);
        thread_t* current = thread_get_current();
        scheduler_suspend_thread(current);
        current->state = WAITING;
        list_remove(&current->link);
        list_append(&item->queue, &current->link);
        thread_yield();

		sem->value--;
	}
    
}

/** Unlocks (ups/signals) the sempahore.
 *
 * Increment the value of this semaphore or wake-up one of blocked threads
 * inside sem_wait().
 *
 * @param sem Semaphore to be unlocked.
 */
void sem_post(sem_t* sem) {
    sem_list_item* item = sem_list_find(sem);
    if (list_get_size(&item->queue)!=0) {
        sem_wake_up_next(item);
    }
    sem->value++;
    thread_yield();
}

/** Try to lock the semaphore without waiting.
 *
 * If the call to sem_wait() would block, do nothing and return EBUSY.
 *
 * @param sem Semaphore to be locked.
 * @return Error code.
 * @retval EOK Semaphore was successfully locked.
 * @retval EBUSY Semaphore has value of 0 and locking would block.
 */
errno_t sem_trywait(sem_t* sem) {
    if (sem->value <= 0) {
        return EBUSY;
	} else {
        sem->value--;
        return EOK;
	}
}
