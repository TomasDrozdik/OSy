// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Check that mutex_trylock returns EBUSY when mutex is locked.
 */

#include <ktest.h>
#include <proc/mutex.h>
#include <proc/thread.h>

static mutex_t mutex;

static void* try_worker(void* ignored) {
    errno_t err = mutex_trylock(&mutex);
    ktest_assert(err == EBUSY, "expected mutex to be busy, got %s", errno_as_str(err));

    return NULL;
}

void kernel_test(void) {
    ktest_start("mutex/try_busy");

    errno_t err = mutex_init(&mutex);
    ktest_assert_errno(err, "mutex_init");

    mutex_lock(&mutex);

    thread_t* worker;
    err = thread_create(&worker, try_worker, NULL, 0, "worker");
    ktest_assert_errno(err, "thread_create");

    err = thread_join(worker, NULL);
    ktest_assert_errno(err, "thread_join");

    mutex_unlock(&mutex);
    mutex_destroy(&mutex);

    ktest_passed();
}
