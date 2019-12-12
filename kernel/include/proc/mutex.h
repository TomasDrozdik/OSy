// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _PROC_MUTEX_H
#define _PROC_MUTEX_H

#include <adt/list.h>
#include <errno.h>
#include <proc/thread.h>
#include <proc/scheduler.h>
#include <types.h>

/** Mutex synchronization primitive. */
typedef struct {
    bool locked;
} mutex_t;

errno_t mutex_init(mutex_t* mutex);
void mutex_destroy(mutex_t* mutex);
void mutex_lock(mutex_t* mutex);
void mutex_unlock(mutex_t* mutex);
errno_t mutex_trylock(mutex_t* mutex);

#endif
