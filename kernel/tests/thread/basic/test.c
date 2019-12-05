// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Basic thread test. Creates one thread that calls yield() several times
 * and then terminates.
 */

#include <ktest.h>
#include <proc/thread.h>

#define LOOPS 5

static void* empty_worker(void* ignored) {
    for (int i = 0; i < LOOPS; i++) {
        thread_yield();
    }
    return NULL;
}

void kernel_test(void) {
    ktest_start("thread/basic");

    thread_t* worker;
    int rc = thread_create(&worker, empty_worker, NULL, 0, "test-worker");
    ktest_assert_errno(rc, "thread_create");

    thread_t* worker2;
    rc = thread_create(&worker2, empty_worker, NULL, 0, "new-workerr");
    ktest_assert_errno(rc, "thread_create");

    rc = thread_join(worker, NULL);
    ktest_assert_errno(rc, "thread_join");

    rc = thread_join(worker2, NULL);
    ktest_assert_errno(rc, "thread_join");

    ktest_passed();
}
