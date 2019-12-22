// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <exc.h>
#include <mm/heap.h>
#include <proc/sem.h>

bool sem_first = true;

//Wake up the next thread in semaphore queue
static void sem_wake_up_next(sem_t* sem) {
    bool enable = interrupts_disable();

    thread_t* next_thread = list_item(list_pop(&sem->queue), thread_t, link);
    next_thread->state = READY;
    scheduler_add_ready_thread(next_thread);

    interrupts_restore(enable);
}

/** Initializes given semaphore.
 *
 * @param sem Semaphore to initialize.
 * @param value Initial semaphore value (1 effectively creates mutex).
 * @return Error code.
 * @retval EOK Semaphore was successfully initialized.
 */
errno_t sem_init(sem_t* sem, int value) {
    bool enable = interrupts_disable();

    sem->value = value;
    list_init(&sem->queue);

    interrupts_restore(enable);
    return EOK;
}

/** Destroys given semaphore.
 *
 * The function must panic if there are threads waiting for this semaphore.
 *
 * @param sem Semaphore to destroy.
 */
void sem_destroy(sem_t* sem) {
    bool enable = interrupts_disable();

    panic_if(list_get_size(&sem->queue) != 0, "Threads still waiting for semaphore");

    interrupts_restore(enable);
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
    bool enable = interrupts_disable();

    while (sem_trywait(sem) == EBUSY) {
        thread_t* current = thread_get_current();
        scheduler_suspend_thread(current);
        current->state = WAITING;
        list_remove(&current->link);
        list_append(&sem->queue, &current->link);

        thread_yield();
    }

    interrupts_restore(enable);
}

/** Unlocks (ups/signals) the sempahore.
 *
 * Increment the value of this semaphore or wake-up one of blocked threads
 * inside sem_wait().
 *
 * @param sem Semaphore to be unlocked.
 */
void sem_post(sem_t* sem) {
    bool enable = interrupts_disable();

    if (list_get_size(&sem->queue) != 0) {
        sem_wake_up_next(sem);
    }

    sem->value++;
    thread_yield();

    interrupts_restore(enable);
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
    bool enable = interrupts_disable();

    if (sem->value <= 0) {
        interrupts_restore(enable);
        return EBUSY;
    } else {
        sem->value--;
        interrupts_restore(enable);
        return EOK;
    }
}
