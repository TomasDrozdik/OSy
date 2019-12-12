// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Try to unlock a mutex in different thread, expecting panic.
 */

#include <ktest.h>
#include <proc/mutex.h>
#include <proc/thread.h>

static mutex_t mutex;

static void* unlocking_worker(void* ignored) {
    mutex_unlock(&mutex);

    ktest_failed();

    return NULL;
}

void kernel_test(void) {
    ktest_start("mutex/bad_unlock");
    ktest_expect_panic();

    errno_t err = mutex_init(&mutex);
    ktest_assert_errno(err, "mutex_init");
    mutex_lock(&mutex);

    thread_t* worker;
    err = thread_create(&worker, unlocking_worker, NULL, 0, "unlocking_worker");
    ktest_assert_errno(err, "thread_create");

    err = thread_join(worker, NULL);
    ktest_assert_errno(err, "thread_join(worker)");

    ktest_failed();
}
