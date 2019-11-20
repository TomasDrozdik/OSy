// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Test for checking that thread return values are correctly retrieved
 * when the thread is joined. Both "return" and thread_finish() is tested.
 */

#include <ktest.h>
#include <proc/thread.h>

static int ret_one, ret_two, ret_three;

static void* worker_one(void* ignored) {
    return &ret_one;
}

static void* worker_two(void* ignored) {
    return &ret_two;
}

static void* worker_three(void* ignored) {
    thread_finish(&ret_three);
}

void kernel_test(void) {
    ktest_start("thread/retvals");

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

    void* retval;
    err = thread_join(one, &retval);
    ktest_assert_errno(err, "thread_join(one)");
    ktest_assert(retval == &ret_one, "worker_one returned invalid value");

    err = thread_join(two, &retval);
    ktest_assert_errno(err, "thread_join(two)");
    ktest_assert(retval == &ret_two, "worker_two returned invalid value");

    err = thread_join(three, &retval);
    ktest_assert_errno(err, "thread_join(three)");
    ktest_assert(retval == &ret_three, "worker_three returned invalid value");

    ktest_passed();
}
