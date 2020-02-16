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

static inline void syscall_putchar(const char c) {
    //dprintk("Printing char.\n");

	printer_putchar(c);
}

static inline unative_t syscall_write(const char* s) {
    //dprintk("Writing to standard output\n");
	
	int counter = 0;
	while (*s != '\0') {
		counter++;
        printer_putchar(*s);
        s++;
	}
    //dprintk("Returning counter = %d\n", counter);
	return counter;
}

static inline unative_t syscall_get_info(np_proc_info_t* info) {
    dprintk("Getting info from thread.\n");
    process_t* process = thread_get_current()->process;
	
	if (info == NULL) {
        printk("Info structure not initialized.");
		return 3;
	}

    info->id = process->id;
    info->virt_mem_size = process->memory_size;
    info->total_ticks = process->total_ticks;

	return info->id;
}

void handle_syscall(context_t* context) {
    syscall_t id = (syscall_t)(context->v0);
    //dprintk("Handling syscall %d\n", id);
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
        //dprintk("Write succesfull.\n");
		break;
	case SYSCALL_PUTCHAR:
        syscall_putchar((char)context->a0);
        //dprintk("Putchar successfull.\n");
		break;
    case SYSCALL_INFO:
        syscall_get_info((np_proc_info_t*)context->a0);
        //dprintk("Got info successfully.\n");
		break;
    default:
        printk("Invalid syscall: %d.\n", id);
    }

    // Upon sucess, shift EPC by 4 to move to the next instruction
    // (unlike e.g. TLBL, we do not want to restart it).
    //dprintk("Shifting epc.\n");
    context->epc += 4;
}

