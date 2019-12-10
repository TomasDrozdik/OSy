// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <drivers/timer.h>
#include <exc.h>
#include <lib/print.h>
#include <drivers/machine.h>
#include <proc/scheduler.h>

void handle_exception_general(context_t* context) {
    timer_interrupt_after(CYCLES);
    scheduler_schedule_next();
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
