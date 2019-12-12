// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Check that sem_trywait returns EBUSY when semaphore is locked.
 */

#include <ktest.h>
#include <proc/sem.h>
#include <proc/thread.h>

static sem_t sem;

static void* try_worker(void* ignored) {
    errno_t err = sem_trywait(&sem);
    ktest_assert(err == EBUSY, "expected semaphore to be busy, got %s", errno_as_str(err));

    return NULL;
}

void kernel_test(void) {
    ktest_start("sem/try_busy");

    errno_t err = sem_init(&sem, 1);
    ktest_assert_errno(err, "sem_init");

    sem_wait(&sem);

    thread_t* worker;
    err = thread_create(&worker, try_worker, NULL, 0, "worker");
    ktest_assert_errno(err, "thread_create");

    err = thread_join(worker, NULL);
    ktest_assert_errno(err, "thread_join");

    sem_post(&sem);
    sem_destroy(&sem);

    ktest_passed();
}
