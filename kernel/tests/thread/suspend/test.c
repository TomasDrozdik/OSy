// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Basic test that checks that thread_suspend() does not return immediatelly.
 */

#include <ktest.h>
#include <proc/thread.h>

#define SAFETY_LOOPS 10

static volatile bool marker_one = false;
static volatile bool marker_two = false;
static volatile bool marker_three = false;

static thread_t* suspended_thread = NULL;
static volatile bool marker = false;
static volatile bool terminate_idle = false;

static void* worker_suspend(void* ignored) {
    marker = true;
    thread_suspend();
    ktest_assert(!marker, "marker should be false (set from different thread)");
    return NULL;
}

static void* worker_wake_up(void* ignored) {
    while (!marker) {
        thread_yield();
    }

    /*
	 * Now, worker_suspend should be already suspended.
	 * But let's wait a bit to be sure.
	 */
    for (int i = 0; i < SAFETY_LOOPS; i++) {
        thread_yield();
    }

    marker = false;
    errno_t err = thread_wakeup(suspended_thread);
    ktest_assert_errno(err, "thread_wakeup(suspended_thread)");

    return NULL;
}

static void* worker_idle(void* ignored) {
    while (!terminate_idle) {
        thread_yield();
    }
    return NULL;
}

void kernel_test(void) {
    ktest_start("thread/suspend");

    errno_t err;

    err = thread_create(&suspended_thread, worker_suspend, NULL, 0, "test_suspend");
    ktest_assert_errno(err, "thread_create(suspend)");

    thread_t* wake_upper;
    err = thread_create(&wake_upper, worker_wake_up, NULL, 0, "test_wake_up");
    ktest_assert_errno(err, "thread_create(wake_up)");

    thread_t* idle;
    err = thread_create(&idle, worker_idle, NULL, 0, "test_idle");
    ktest_assert_errno(err, "thread_create(idle)");

    err = thread_join(suspended_thread, NULL);
    ktest_assert_errno(err, "thread_join(suspend)");

    err = thread_join(wake_upper, NULL);
    ktest_assert_errno(err, "thread_join(wake_up)");

    terminate_idle = true;
    err = thread_join(idle, NULL);
    ktest_assert_errno(err, "thread_join(idle)");

    ktest_passed();
}
