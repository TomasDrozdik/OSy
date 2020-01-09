// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _MM_FRAME_H
#define _MM_FRAME_H

#include <errno.h>
#include <types.h>

#define FRAME_SIZE 4096

#define KSEG0_BASE 0x80000000

#define KSEG0_TO_PHYS(kseg0_addr) \
    ((kseg0_addr)-KSEG0_BASE)

#define PHYS_TO_KSEG0(phys_addr) \
    ((phys_addr) + KSEG0_BASE)

void frame_init(void);
errno_t frame_alloc(size_t count, uintptr_t* phys);
errno_t frame_free(size_t count, uintptr_t phys);

size_t get_page_count(void);
void debug_print_paging(void);

#endif
