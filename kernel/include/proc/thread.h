// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _PROC_THREAD_H
#define _PROC_THREAD_H

#include <errno.h>
#include <types.h>
#include <adt/list.h>
#include <proc/context.h>

/** Thread stack size.
 *
 * Set quite liberally as stack overflows are notoriously difficult to debug
 * (and difficult to detect too).
 */
#define THREAD_STACK_SIZE 4096

/** Max length (excluding terminating zero) of thread name. */
#define THREAD_NAME_MAX_LENGTH 31

#define THREAD_INITIAL_STACK_TOP(THREADPTR) \
    ((unative_t)((uintptr_t)THREADPTR + sizeof(thread_t) + THREAD_STACK_SIZE))

#define THREAD_INITIAL_CONTEXT(THREADPTR) \
    ((context_t*)(THREAD_INITIAL_STACK_TOP(THREADPTR) - sizeof(context_t)))

/** Thread entry function as you know from pthreads. */
typedef void* (*thread_entry_func_t)(void*);

/** State of a thread in a scheduler. */
typedef enum thread_state {
    READY,
    SUSPENDED,
    FINISHED,
} thread_state_t;

/** Information about any existing thread. */
typedef struct thread thread_t;
struct thread {
    char name[THREAD_NAME_MAX_LENGTH + 1];
    thread_entry_func_t entry_func;
    void * data;
    void* retval;
    thread_state_t state;
    link_t link;

    // Pointer to where context (i.e. stack top in terms of cpu_context_switch)
    // is stored.
    context_t* context;
};

void threads_init(void);
errno_t thread_create(thread_t** thread, thread_entry_func_t entry, void* data, unsigned int flags, const char* name);
thread_t* thread_get_current(void);
void thread_yield(void);
void thread_suspend(void);
void thread_finish(void* retval) __attribute__((noreturn));
bool thread_has_finished(thread_t* thread);
errno_t thread_wakeup(thread_t* thread);
errno_t thread_join(thread_t* thread, void** retval);
void thread_switch_to(thread_t* thread);

#endif
