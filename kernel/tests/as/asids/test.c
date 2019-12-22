// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Tests that different AS are separated from each other.
 */

#include <drivers/machine.h>
#include <ktest.h>
#include <mm/as.h>
#include <proc/sem.h>
#include <proc/thread.h>

/*
 * Increase to 260 if you recycle ASIDs of active address
 * spaces ;-)
 */
#define TEST_AS_COUNT 20

#define LOOPS 100

typedef struct {
    thread_t* thread;
    unative_t pattern;
} worker_info_t;

static worker_info_t worker_info[TEST_AS_COUNT];
static sem_t started_worker_counter;
static volatile bool worker_can_run = false;

static void* as_worker(void* info_arg) {
    worker_info_t* info = info_arg;

    sem_wait(&started_worker_counter);

    while (!worker_can_run) {
        thread_yield();
    }

    volatile unative_t* data = (unative_t*)(PAGE_SIZE);

    unative_t i = 0;
    do {
        unative_t expected_pattern = info->pattern ^ i;
        *data = expected_pattern;

        thread_yield();

        ktest_assert(*data == expected_pattern,
                "%pT: value mismatch (base_pattern=0x%x, i=0x%x, actual=0x%x, expected=0x%x)",
                info->thread, info->pattern, i, *data, expected_pattern);

        i++;
    } while (i < LOOPS);

    return NULL;
}

void kernel_test(void) {
    ktest_start("as/asids");

    errno_t err = sem_init(&started_worker_counter, TEST_AS_COUNT);
    ktest_assert_errno(err, "sem_init");

    for (size_t i = 0; i < TEST_AS_COUNT; i++) {
        worker_info[i].pattern = i;
        err = thread_create_new_as(&worker_info[i].thread, as_worker, &worker_info[i], 0, "worker", PAGE_SIZE * 2);
        ktest_assert_errno(err, "thread_create");
    }

    while (sem_trywait(&started_worker_counter) != EBUSY) {
        sem_post(&started_worker_counter);
    }

    dprintk("All workers are up.\n");
    worker_can_run = true;

    for (size_t i = 0; i < TEST_AS_COUNT; i++) {
        worker_info[i].pattern = i;
        err = thread_join(worker_info[i].thread, NULL);
        ktest_assert_errno(err, "thread_join");
    }

    ktest_passed();
}
