// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _PROC_SCHEDULER_H
#define _PROC_SCHEDULER_H

#include <proc/thread.h>

void scheduler_init(void);

void scheduler_add_ready_thread(thread_t* id);

void scheduler_remove_thread(thread_t* id);

void scheduler_remove_current_thread(void);

void scheduler_suspend_thread(thread_t* id);

void scheduler_suspend_current_thread(void);

void scheduler_schedule_next(void);

thread_t* scheduler_get_running_thread(void);

#endif
