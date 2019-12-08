// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _MM_HEAP_H
#define _MM_HEAP_H

#include <types.h>

/** Initialized heap.
 *
 * Uses pointer to kernel end and size of the available memory to determine
 * pointer to the end of available memory i.e. heap.
 */
void heap_init(void);

void* kmalloc(size_t size);
void kfree(void* ptr);

#endif
