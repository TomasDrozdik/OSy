// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug/mm.h>
#include <main.h>

/** Return pointer to memory after kernel. */
uintptr_t debug_get_kernel_endptr(void) {
    return (uintptr_t)&_kernel_end;
}

/** Probe available base physical memory.
 *
 * Do not check for non-continuous memory blocks or for memory available via
 * TLB only.
 *
 * @return Amount of memory available in bytes.
 */
size_t debug_get_base_memory_size(void) {
    return debug_get_base_memory_endptr() - debug_get_kernel_endptr();
}

// TODO: implement binary search for this
uintptr_t debug_get_base_memory_endptr(void) {
    // Go through the memory try to assign something then read it.
    // If the read value is what we've assigned before then we consider this a
    // valid memory block.
    volatile uintptr_t* addr = (uintptr_t*)debug_get_kernel_endptr();

    // Shift ammount, consider it's pointer arithmetics.
    size_t shift_ammount = 1024 / sizeof(addr);

    // To store original value.
	uintptr_t prev_value;

    while (true) {
        addr += shift_ammount;
        prev_value = *addr; // Store previous value.
        // This is why addr is volatile, if it weren't compiler would assume
        // that what we assigned must be there.
        *addr = (uintptr_t)addr;
        if (*addr != (uintptr_t)addr) {
            break;
        }
        *addr = prev_value; // Restore original value.
    }
    return (uintptr_t)(addr - shift_ammount);
}
