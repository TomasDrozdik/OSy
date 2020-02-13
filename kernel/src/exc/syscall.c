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
    thread_finish((void *)exitcode);
    // Noreturn call.
    panic("Reached noreturn path.\n");
    while(1)
        ;
}

void handle_syscall(context_t* context) {
    syscall_t id = (syscall_t)(context->v0);
    dprintk("Handling syscall %d\n", id);
    switch (id) {
    case SYSCALL_EXIT:
        syscall_exit(context->a0);

        // Noreturn call.
        panic("Reached noreturn path.\n");
        while(1)
            ;
        break;
    default:
        panic("Invalid syscall: %d.\n", id);
    }

    // Upon sucess, shift EPC by 4 to move to the next instruction
    // (unlike e.g. TLBL, we do not want to restart it).
    context->epc += 4;
}

