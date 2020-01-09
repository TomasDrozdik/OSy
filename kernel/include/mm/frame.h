// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _MM_FRAME_H
#define _MM_FRAME_H

#include <errno.h>
#include <types.h>

#define FRAME_SIZE 4096

void frame_init(void);
errno_t frame_alloc(size_t count, uintptr_t* phys);
errno_t kframe_alloc(size_t count, uintptr_t* kseg0ptr);

errno_t frame_free(size_t count, uintptr_t phys);
errno_t kframe_free(size_t count, uintptr_t kseg0ptr);

size_t get_page_count(void);
void debug_print_paging(void);

#endif
