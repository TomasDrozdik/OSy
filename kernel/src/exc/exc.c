// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <drivers/machine.h>
#include <drivers/timer.h>
#include <exc.h>
#include <lib/print.h>
#include <proc/scheduler.h>
#include <proc/userspace.h>

/* General exception code value mnemonics. */
#define Int  0  /* Interrupt. */
#define TLBL 2  /* TLB exception (load or instruction fetch). */
#define TLBS 3  /* TLB exception (store). */
#define AdEL 4  /* Address error exception (load or instruction fetch). */
#define Sys  8  /* Syscall. */
#define CpU  11 /* Coprocessor unusable exception. */

/* Interrupt code mnemonics. */
#define CLK  7  /* Clock interrupt. */

static void handle_interrupt(context_t* context) {
    if (cp0_cause_is_interrupt_pending(context->cause, CLK)) {
        timer_interrupt_after(CYCLES);
        scheduler_schedule_next();
    }
}

void handle_exception_general(context_t* context) {
    unative_t exc = cp0_cause_get_exc_code(context->cause);
    switch (exc) {
    case Int:
        dprintk("Interrupt.. cause:%u, status: %x, epc: %x\n",
                context->cause, context->status, context->epc);
        handle_interrupt(context);
        return;
    case AdEL:
        dprintk("Address error exception (load or instruction fetch).. status: %x, epc: %x\n",
                context->status, context->epc);
        thread_kill(thread_get_current());
        return;
    case TLBL:
        dprintk("TLB exception (load or instruction fetch).. status: %x, epc: %x\n",
                context->status, context->epc);
        thread_kill(thread_get_current());
        return;
    case TLBS:
        dprintk("TLBL / TLBS exception -> killing current thread.\n");
        thread_kill(thread_get_current());
        return;
    case Sys:
        dprintk("Syscall.. status: %x, epc: %x\n",
                context->status, context->epc);
        handle_syscall(context);
        return;
    case CpU:
        dprintk("Coprocessor unsusable exception.. status: %x, epc: %x\n",
                context->status, context->epc);
        thread_kill(thread_get_current());
        return;
    default:
        panic("Exception...%d, status: %x, epc: %x\n", exc, context->status, context->epc);
    }
}

bool interrupts_disable(void) {
    unative_t status = cp0_read(REG_CP0_STATUS);
    cp0_write(REG_CP0_STATUS, status & ~CP0_STATUS_IE_BIT);
    return (status & CP0_STATUS_IE_BIT) > 0;
}

void interrupts_restore(bool enable) {
    unative_t status = cp0_read(REG_CP0_STATUS);
    if (enable) {
        status = status | CP0_STATUS_IE_BIT;
    } else {
        status = status & ~CP0_STATUS_IE_BIT;
    }
    cp0_write(REG_CP0_STATUS, status);
}
