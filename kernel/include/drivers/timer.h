// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _DRIVERS_TIMER_H_
#define _DRIVERS_TIMER_H_

#include <drivers/cp0.h>
#include <types.h>

//Number of cycles of one process
#define CYCLES 100000

static inline void timer_interrupt_after(unative_t cycles) {
    cp0_write_compare(cp0_read_count() + cycles);
}

#endif
