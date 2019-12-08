// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Basic test that thread code is actually executed.
 */

#include <ktest.h>
#include <proc/thread.h>

static volatile bool marker_one = false;
static volatile bool marker_two = false;
static volatile bool marker_three = false;

static void* worker_one(void* ignored) {
    marker_one = true;
    return NULL;
}

static void* worker_two(void* ignored) {
    marker_two = true;
    return NULL;
}

static void* worker_three(void* ignored) {
    marker_three = true;
    return NULL;
}

void kernel_test(void) {
    ktest_start("thread/running");

    errno_t err;

    thread_t* one;
    err = thread_create(&one, worker_one, NULL, 0, "test_one");
    ktest_assert_errno(err, "thread_create(one)");

    thread_t* two;
    err = thread_create(&two, worker_two, NULL, 0, "test_two");
    ktest_assert_errno(err, "thread_create(two)");

    thread_t* three;
    err = thread_create(&three, worker_three, NULL, 0, "test_three");
    ktest_assert_errno(err, "thread_create(three)");

    err = thread_join(one, NULL);
    ktest_assert_errno(err, "thread_join(one)");

    err = thread_join(two, NULL);
    ktest_assert_errno(err, "thread_join(two)");

    err = thread_join(three, NULL);
    ktest_assert_errno(err, "thread_join(three)");

    // Check that the workers have actually run

    ktest_assert(marker_one, "worker one was not executed");
    ktest_assert(marker_two, "worker two was not executed");
    ktest_assert(marker_three, "worker three was not executed");

    ktest_passed();
}
