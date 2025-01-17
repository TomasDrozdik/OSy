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

void kernel_test(void) {
    ktest_start("heap/basic");

    void* ptr = kmalloc(8);
    void* tmp = ptr;
    ktest_assert(ptr != NULL, "no memory available");
    ktest_check_kmalloc_result(ptr, 8);

    kfree(ptr);

    ptr = kmalloc(8);

    ktest_assert(tmp == ptr, "Weird assignment methodology.");

    ktest_passed();
}
