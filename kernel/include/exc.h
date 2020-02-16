// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _EXC_H
#define _EXC_H

#include <proc/context.h>

/** Available system calls.
 *
 * Must be kept up-to-date with userspace list.
 */
typedef enum {
    SYSCALL_EXIT,
	SYSCALL_PUTCHAR,
    SYSCALL_WRITE,
	SYSCALL_INFO,
    SYSCALL_LAST
} syscall_t;

void handle_syscall(context_t* context);
void handle_exception_general(context_t* context);
bool interrupts_disable(void);
void interrupts_restore(bool enable);

#endif
