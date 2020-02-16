// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _LIBC_STDLIB_H
#define _LIBC_STDLIB_H

#include <stddef.h>
#include <np/types.h>

void exit(int status);
void* malloc(size_t size);
void free(void* ptr);

uintptr_t debug_get_app_endptr(void);
size_t debug_get_base_memory_size(void);
uintptr_t debug_get_base_memory_endptr(void);

#endif
