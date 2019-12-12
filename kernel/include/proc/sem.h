// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _PROC_SEM_H
#define _PROC_SEM_H

/*
 * Semaphore synchronization primitive. Note that the naming follows
 * System V convention, i.e. wait decrements (locks) the counter, post
 * signals incrementing (unlock).
 */

#include <adt/list.h>
#include <errno.h>
#include <types.h>
#include <proc/scheduler.h>
#include <proc/thread.h>

typedef struct {
    int value;
} sem_t;

typedef struct list_item {
    link_t link;
    sem_t* semaphore;
    list_t queue;
} sem_list_item;

sem_list_item* sem_list_find(sem_t* sem);
void sem_wake_up_next(sem_list_item* item);
errno_t sem_init(sem_t* sem, int value);
void sem_destroy(sem_t* sem);
int sem_get_value(sem_t* sem);
void sem_post(sem_t* sem);
void sem_wait(sem_t* sem);
errno_t sem_trywait(sem_t* sem);

#endif
