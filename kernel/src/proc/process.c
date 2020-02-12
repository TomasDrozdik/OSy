// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <errno.h>
#include <mm/as.h>
#include <mm/heap.h>
#include <proc/process.h>
#include <proc/userspace.h>
#include <utils.h>

#define PROCESS_STACK_SIZE (3 * 4096)

#define PROCESS_INITIAL_STACK_TOP \
    (INITIAL_VIRTUAL_ADDRESS + PROCESS_STACK_SIZE - 4)

#define PROCESS_ENTRYPOINT 0x00004000

static void *process_load(void *process_ptr) {
    process_t *process = (process_t *)process_ptr;

    // First copy the binary executable image to assigned virtual memory.
    memcpy(PROCESS_ENTRYPOINT, process->image_location + PROCESS_ENTRYPOINT,
            process->image_size - PROCESS_ENTRYPOINT);

    // Now switch context to userspace thread.
    dprintk("Executing context switch to userspace.\n");
    cpu_jump_to_userspace(PROCESS_INITIAL_STACK_TOP, PROCESS_ENTRYPOINT);

    // TODO: Return value form main somehow..
    return NULL;
}

/** Create new userspace process.
 *
 * @param process_out Where to place the initialized process_t structure.
 * @param image_location Virtual address (in kernel segment) where is the image of the raw application binary.
 * @param image_size Size of the application binary image.
 * @param process_memory_size Amount of virtual memory to give to the application (at least image_size).
 * @return Error code.
 * @retval EOK Process was created and its main thread started.
 * @retval ENOMEM Not enough memory to complete the operation.
 * @retval EINVAL Invalid call (unaligned size, process memory < image size).
 */
errno_t process_create(process_t** process_out, uintptr_t image_location, size_t image_size, size_t process_memory_size) {
    if (process_memory_size < image_size ||
            process_memory_size % PAGE_SIZE != 0) {
        return EINVAL;
    }

    process_t* process = (process_t*)kmalloc(sizeof(process_t));
    if (process == NULL) {
        return ENOMEM;
    }

    // Memorize these parameters for process_load function.
    process->image_location = image_location;
    process->image_size = image_size;
    process->memory_size = process_memory_size;

    errno_t err = thread_create_new_as(&process->thread, process_load,
            (void *)process, 0, "UAPP", process_memory_size);
    switch (err) {
    case EOK:
        break;
    case ENOMEM:
        kfree(process);
        return ENOMEM;
    default:
        panic("Invalid execution path.");
    }

    *process_out = process;
    return EOK;
}

/** Wait for termination of another process.
 *
 * @param process Process to wait for.
 * @param exit_status Where to place the process exit status (return value from main).
 * @return Error code.
 * @retval EOK Joined successfully.
 * @retval EBUSY Some other thread is already joining this process.
 * @retval EKILLED Process was killed.
 * @retval EINVAL Invalid process.
 */
errno_t process_join(process_t* process, int* exit_status) {
    errno_t err = thread_join(process->thread, (void **)&exit_status);
    switch (err) {
    case EOK:
        break;
    case EBUSY:
    case EKILLED:
    case EINVAL:
        return err;
    default:
        panic("Invalid execution path.");
    }
    return EOK;
}

