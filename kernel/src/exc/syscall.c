// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <drivers/timer.h>
#include <exc.h>
#include <lib/print.h>
#include <proc/thread.h>
#include <drivers/printer.h>

static inline void syscall_exit(int exitcode) {
    dprintk("exit code: %d\n", exitcode);
    // HACK: to prevent allocation of single int we just return exit code as
    // void*.
    thread_finish((void *)exitcode);
    // Noreturn call.
    assert(0 && "Reached noreturn path.");
    while(1)
        ;
}

static inline unative_t syscall_write(const char* s) {
    dprintk("Writing to standard output\n");
	
	int counter = 0;
	while (*s != '\0') {
		counter++;
        printer_putchar(*s);
        s++;
	}

	return counter;
}

void handle_syscall(context_t* context) {
    syscall_t id = (syscall_t)(context->v0);
    dprintk("Handling syscall %d\n", id);
    switch (id) {
    case SYSCALL_EXIT:
        syscall_exit((int)context->a0);
        // Noreturn call.
        assert(0 && "Reached noreturn path.");
        while(1)
            ;
        break;
    case SYSCALL_WRITE:
        context->v0=syscall_write((char*)context->a0);

		break;
    default:
        panic("Invalid syscall: %d.\n", id);
    }

    // Upon sucess, shift EPC by 4 to move to the next instruction
    // (unlike e.g. TLBL, we do not want to restart it).
    context->epc += 4;
}

