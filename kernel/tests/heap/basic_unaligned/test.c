// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include "../theap.h"
#include <ktest.h>
#include <mm/heap.h>

/*
 * Tests that kmalloc() returns a valid address. We assume
 * there would be always enough free memory to allocate 8 bytes
 * when the kernel starts.
 */

#define ALIGN_SIZE 4
#define MAX_ALLOC_SIZE 100

void kernel_test(void) {
    ktest_start("heap/basic");

    void* ptr;
    for (size_t size = 0; size < MAX_ALLOC_SIZE; size += ALIGN_SIZE) {
        for (size_t misaligned = 0; misaligned < ALIGN_SIZE; ++misaligned) {
            size_t actual_size = size + misaligned;
            ptr = kmalloc(actual_size);
            ktest_assert(ptr != NULL, "no memory available");
            ktest_check_kmalloc_result(ptr, actual_size);
        }
    }

    ktest_passed();
}
