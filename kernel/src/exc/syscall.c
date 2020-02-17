// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <drivers/timer.h>
#include <exc.h>
#include <lib/print.h>
#include <proc/thread.h>

static inline void syscall_exit(int exitcode) {
    dprintk("exit code: %d\n", exitcode);

    // HACK: to prevent allocation of single int we just return exit code as
    // void*.
    thread_finish((void*)exitcode);

    // Noreturn function.
    assert(0 && "Reached noreturn path.");
    while (1)
        ;
}

// TODO: remove putchar replace with correct write see `man 2 write`
static inline void syscall_putchar(const char c) {
    putchar(c);
}

static inline unative_t syscall_write(const char* s) {
    return fputs(s);
    ;
}

static inline unative_t syscall_get_info(np_proc_info_t* info) {
    thread_t* thread = thread_get_current();
    process_t* process = thread->process;

    if (info == NULL) {
        dprintk("Info structure not initialized.\n");
        return 3;
    }

    info->id = process->id;
    info->virt_mem_size = process->memory_size;
    info->total_ticks = ++process->total_ticks;

    return info->id;
}

errno_t handle_syscall(context_t* context) {
    syscall_t id = (syscall_t)context->v0;
    switch (id) {
    case SYSCALL_EXIT:
        syscall_exit((int)context->a0); // Noreturn call.
        break;
    case SYSCALL_WRITE:
        context->v0 = syscall_write((char*)context->a0);
        break;
    case SYSCALL_PUTCHAR:
        syscall_putchar((char)context->a0);
        break;
    case SYSCALL_INFO:
        context->v0 = syscall_get_info((np_proc_info_t*)context->a0);
        break;
    default:
        dprintk("Invalid syscall: %d.\n", id);
        return EINVAL;
    }

    // Upon sucess, shift EPC by 4 to move to the next instruction
    // (unlike e.g. TLBL, we do not want to restart it).
    context->epc += 4;
    return EOK;
}
