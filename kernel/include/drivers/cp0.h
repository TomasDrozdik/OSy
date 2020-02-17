// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _DRIVERS_CP0_H
#define _DRIVERS_CP0_H

#define CP0_STATUS_IE_BIT 0x1

#define REG_CP0_BADVADDR 8
#define REG_CP0_STATUS 12
#define REG_CP0_CAUSE 13
#define REG_CP0_EPC 14

#define cp0_build_bitfield(value, width, shift) \
    (((value) & ((1 << (width)) - 1)) << (shift))

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
#define cp0_write_pagemask_4k() cp0_write(5, 0)

#define cp0_write_index(value) cp0_write(0, value)
#define cp0_write_entrylo0(pfn, dirty, valid, global) \
    cp0_write(2, \
            cp0_build_bitfield(global, 1, 0) \
                    | cp0_build_bitfield(valid, 1, 1) \
                    | cp0_build_bitfield(dirty, 1, 2) \
                    | cp0_build_bitfield(0, 3, 3) \
                    | cp0_build_bitfield(pfn, 24, 6))
#define cp0_write_entrylo1(pfn, dirty, valid, global) \
    cp0_write(3, \
            cp0_build_bitfield(global, 1, 0) \
                    | cp0_build_bitfield(valid, 1, 1) \
                    | cp0_build_bitfield(dirty, 1, 2) \
                    | cp0_build_bitfield(0, 3, 3) \
                    | cp0_build_bitfield(pfn, 24, 6))

#define cp0_write_entryhi(vpn2, asid) \
    cp0_write(10, cp0_build_bitfield(vpn2, 19, 13) | cp0_build_bitfield(asid, 8, 0))

#define cp0_tlb_write_random() __asm__ volatile("tlbwr\n")
#define cp0_tlb_write_indexed() __asm__ volatile("tlbwi\n")
#define cp0_tlb_read() __asm__ volatile("tlbr\n")

#ifndef __ASSEMBLER__
#include <types.h>

static inline unsigned int cp0_cause_get_exc_code(unative_t status) {
    return (status >> 2) & 0x1F;
}

static inline bool cp0_cause_is_interrupt_pending(unative_t status, unsigned int intr) {
    return (status >> 8) & (1 << intr);
}

static inline uint8_t cp0_tlb_read_entry_asid(size_t index) {
    cp0_write_index(index);
    cp0_tlb_read();
    return cp0_read(10);
}

static inline void cp0_tlb_read_entry_4k_page(size_t index, unative_t* vp_entry, unative_t* pf_entry_even, unative_t* pf_entry_odd) {
    cp0_write_index(index);
    cp0_tlb_read();
    *vp_entry = cp0_read(10);
    *pf_entry_even = cp0_read(2);
    *pf_entry_odd = cp0_read(3);
}

#endif

#endif
