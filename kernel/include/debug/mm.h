// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _DEBUG_MM_H
#define _DEBUG_MM_H

#include <types.h>

uintptr_t debug_get_kernel_endptr(void);

size_t debug_get_base_memory_size(void);

uintptr_t debug_get_base_memory_endptr(void);

#endif
