// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _PROC_THREAD_H
#define _PROC_THREAD_H

#include <adt/list.h>
#include <errno.h>
#include <proc/context.h>
#include <types.h>
#include <mm/as.h>

/** Thread stack size.
 *
 * Set quite liberally as stack overflows are notoriously difficult to debug
 * (and difficult to detect too).
 */
#define THREAD_STACK_SIZE 4096

/** Max length (excluding terminating zero) of thread name. */
#define THREAD_NAME_MAX_LENGTH 31

/* Forward declaration to prevent cyclic header inclusions. */
typedef struct as as_t;

/** Thread entry function as you know from pthreads. */
typedef void* (*thread_entry_func_t)(void*);

/** State of a thread in a scheduler. */
typedef enum thread_state {
    READY,
    SUSPENDED,
    FINISHED,
    WAITING,
    KILLED,
} thread_state_t;

/** Information about any existing thread. */
typedef struct {
    char name[THREAD_NAME_MAX_LENGTH + 1];
    thread_entry_func_t entry_func;
    void* data;
    void* retval;
    thread_state_t state;
    link_t link;
    as_t* as;
    uintptr_t stack;

    // Pointer to where context (i.e. stack top in terms of cpu_context_switch)
    // is stored.
    context_t* context;
} thread_t;

void threads_init(void);
errno_t thread_create(thread_t** thread, thread_entry_func_t entry, void* data, unsigned int flags, const char* name);
errno_t thread_create_new_as(thread_t** thread, thread_entry_func_t entry, void* data, unsigned int flags, const char* name, size_t as_size);
thread_t* thread_get_current(void);
void thread_yield(void);
void thread_suspend(void);
void thread_finish(void* retval) __attribute__((noreturn));
bool thread_has_finished(thread_t* thread);
errno_t thread_wakeup(thread_t* thread);
errno_t thread_join(thread_t* thread, void** retval);
void thread_switch_to(thread_t* thread);
as_t* thread_get_as(thread_t* thread);
errno_t thread_kill(thread_t* thread);

#endif
