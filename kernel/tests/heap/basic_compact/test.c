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

#define ALLOCS 4

void kernel_test(void) {
    ktest_start("heap/basic");

    size_t alloc_size = 1024;

    void* ptrs[ALLOCS];

    for (size_t i = 0; i < ALLOCS; ++i) {
        ptrs[i] = kmalloc(alloc_size);
        ktest_assert(ptrs[i] != NULL, "no memory available");
        ktest_check_kmalloc_result(ptrs[i], alloc_size);
    }

    kfree(ptrs[1]);
    kfree(ptrs[0]);
    kfree(ptrs[2]);
    kfree(ptrs[3]);

    // Since all blocks should be deallocated and compacted new malloc should
    // be equal to ptrs[0].
    void* ptr = kmalloc(1024 * ALLOCS);
    ktest_assert(ptrs[0] == ptr, "Compacting error.");

    ktest_passed();
}
