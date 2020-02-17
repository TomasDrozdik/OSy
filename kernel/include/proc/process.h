// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _PROC_PROCESS_H
#define _PROC_PROCESS_H

#include <errno.h>
#include <proc/thread.h>
#include <types.h>

/** Forward declaration of thread_t since process has pointer to its thread
 * but at the same time our design relys on having pointer from thread to its
 * process to be able to get current process from thread_get_current()
 */
typedef struct thread thread_t;

/** Virtual address of the entry point to userspace application. */
#define PROCESS_ENTRY_POINT 0x00004000

/** Virtual address where the application binary is mounted in MSIM. */
#define PROCESS_IMAGE_START 0xBFB00000

/** Size of the application binary. */
#define PROCESS_IMAGE_SIZE (1024 * 128)

#ifndef PROCESS_MEMORY_SIZE
/** Amount of virtual memory to give to the userspace process. */
#define PROCESS_MEMORY_SIZE (PROCESS_IMAGE_SIZE * 2)
#endif

#if PROCESS_MEMORY_SIZE < PROCESS_IMAGE_SIZE
#error "Cannot give less memory than image size!"
#endif

/** Information about existing process. */
typedef struct process {
    unative_t id;
    thread_t* thread;
    uintptr_t image_location;
    size_t image_size;
    size_t memory_size;
    size_t total_ticks;
} process_t;

/** Information about running process.
 *
 * See syscall_np_proc_info_get for details.
 */
typedef struct np_proc_info {
    unative_t id;
    size_t virt_mem_size;
    size_t total_ticks;
} np_proc_info_t;



errno_t process_create(process_t** process, uintptr_t image_location, size_t image_size, size_t process_memory_size);
errno_t process_join(process_t* process, int* exit_status);

#endif
