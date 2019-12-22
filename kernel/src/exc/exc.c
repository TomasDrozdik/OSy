// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <drivers/machine.h>
#include <drivers/timer.h>
#include <exc.h>
#include <lib/print.h>
#include <proc/scheduler.h>

void handle_exception_general(context_t* context) {
    unative_t exc = cp0_cause_get_exc_code(context->cause);
    bool pending = cp0_cause_is_interrupt_pending(context->cause, 7);

    if (exc == 0 && pending == 1) {
        timer_interrupt_after(CYCLES);
        scheduler_schedule_next();
    } else {
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
