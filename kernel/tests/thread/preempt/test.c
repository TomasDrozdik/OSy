// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Test that checks that scheduling is really preemptive.
 */

#include <ktest.h>
#include <proc/thread.h>

static volatile size_t busy_counter = 1 << 8;

static volatile bool can_start_one = false;
static volatile bool can_start_two = false;
static volatile bool can_start_three = false;

#define BUSY_WAIT(var) \
    do { \
        while (!var) { \
        } \
    } while (0)

static void* worker_one(void* ignored) {
    puts(" => worker_one()");
    BUSY_WAIT(can_start_one);
    puts(" <= worker_one()");
    return NULL;
}

static void* worker_two(void* ignored) {
    puts(" => worker_two()");
    BUSY_WAIT(can_start_two);
    can_start_three = true;
    puts(" <= worker_two()");
    return NULL;
}

static void* worker_three(void* ignored) {
    puts(" => worker_three()");
    BUSY_WAIT(can_start_three);
    can_start_one = true;
    puts(" <= worker_three()");
    return NULL;
}

void kernel_test(void) {
    ktest_start("thread/preempt");

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

    while (busy_counter > 0) {
        busy_counter--;
    }
    can_start_two = true;

    void* retval;
    err = thread_join(one, &retval);
    ktest_assert_errno(err, "thread_join(one)");

    err = thread_join(two, &retval);
    ktest_assert_errno(err, "thread_join(two)");

    err = thread_join(three, &retval);
    ktest_assert_errno(err, "thread_join(three)");

    ktest_passed();
}
