// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _DRIVERS_CP0_H
#define _DRIVERS_CP0_H

#define CP0_STATUS_IE_BIT 0x1

#define REG_CP0_BADVADDR 8
#define REG_CP0_STATUS 12
#define REG_CP0_CAUSE 13
#define REG_CP0_EPC 14

#define cp0_quote_me_(x) #x
#define cp0_quote_me(x) cp0_quote_me_(x)

#define cp0_read(register_number) \
    ({ \
        unative_t __result; \
        __asm__ volatile( \
                ".set push \n" \
                ".set noreorder \n" \
                "nop \n" \
                "mfc0 %0, $" cp0_quote_me(register_number) " \n" \
                                                           ".set pop \n" \
                : "=r"(__result)); \
        __result; \
    })

#define cp0_write(register_number, value) \
    __asm__ volatile( \
            ".set push \n" \
            ".set noreorder \n" \
            "nop \n" \
            "mtc0 %0, $" cp0_quote_me(register_number) " \n" \
                                                       ".set pop \n" \
            : \
            : "r"(value))

#define cp0_read_count() cp0_read(9)
#define cp0_write_compare(value) cp0_write(11, value)

#ifndef __ASSEMBLER__
#include <types.h>

static inline unsigned int cp0_cause_get_exc_code(unative_t status) {
    return (status >> 2) & 0x1F;
}

static inline bool cp0_cause_is_interrupt_pending(unative_t status, unsigned int intr) {
    return (status >> 8) & (1 << intr);
}

#endif

#endif
