// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug/mm.h>
#include <main.h>

/** Probe available base physical memory.
 *
 * Do not check for non-continuous memory blocks or for memory available via
 * TLB only.
 *
 * @return Amount of memory available in bytes.
 */
size_t debug_get_base_memory_size(void) {
    return debug_get_base_memory_endptr() - (uintptr_t)&_kernel_end;
}

uintptr_t debug_get_base_memory_endptr(void) {
    // Go through the memory try to assign something then read it.
    // If the read value is what we've assigned before then we consider this a
    // valid memory block.
    volatile uintptr_t* addr = (uintptr_t*)&_kernel_end;
    size_t shift_ammount = 1024;
    uintptr_t prev_value;

    while (true) {
        // Divide by sizeof(addr) to compensate for pointer arithmetics.
        addr += shift_ammount / sizeof(addr);
        prev_value = *addr;  // Store previous value.
        // This is why addr is volatile, if it weren't compiler would assume
        // that what we assigned must be there.
        *addr = (uintptr_t)addr;
        if (*addr != (uintptr_t)addr) {
            break;
        }
        *addr = prev_value;  // Restore original value.
    }
    return (uintptr_t)addr;
}
