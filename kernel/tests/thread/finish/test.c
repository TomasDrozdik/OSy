// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Test for thread_finish() function to check that this function really
 * never returns.
 */

#include <ktest.h>
#include <proc/thread.h>

#define LOOPS 5

static void* empty_worker(void* ignored) {
    thread_finish(NULL);

    ktest_assert(false, "thread survived thread_finish()");
}

void kernel_test(void) {
    ktest_start("thread/finish");

    thread_t* worker;
    int rc = thread_create(&worker, empty_worker, NULL, 0, "test-worker");
    ktest_assert_errno(rc, "thread_create");

    rc = thread_join(worker, NULL);
    ktest_assert_errno(rc, "thread_join");

    ktest_passed();
}
