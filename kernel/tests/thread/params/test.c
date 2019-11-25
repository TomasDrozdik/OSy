// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Test for checking that thread parameters are passed correctly into
 * the threads.
 */

#include <ktest.h>
#include <proc/thread.h>

static int one, two, three;
static void* param_one = &one;
static void* param_two = &two;
static void* param_three = &three;

static void* worker_one(void* param) {
    ktest_assert(param == param_one, "worker_one parameter passed incorrectly");
    return NULL;
}

static void* worker_two(void* param) {
    ktest_assert(param == param_two, "worker_two parameter passed incorrectly");
    return NULL;
}

static void* worker_three(void* param) {
    ktest_assert(param == param_three, "worker_three parameter passed incorrectly");
    return NULL;
}

void kernel_test(void) {
    ktest_start("thread/params");

    errno_t err;

    thread_t* one;
    err = thread_create(&one, worker_one, param_one, 0, "test_one");
    ktest_assert_errno(err, "thread_create(one)");

    thread_t* two;
    err = thread_create(&two, worker_two, param_two, 0, "test_two");
    ktest_assert_errno(err, "thread_create(two)");

    thread_t* three;
    err = thread_create(&three, worker_three, param_three, 0, "test_three");
    ktest_assert_errno(err, "thread_create(three)");

    err = thread_join(one, NULL);
    ktest_assert_errno(err, "thread_join(one)");

    err = thread_join(two, NULL);
    ktest_assert_errno(err, "thread_join(two)");

    err = thread_join(three, NULL);
    ktest_assert_errno(err, "thread_join(three)");

    ktest_passed();
}
