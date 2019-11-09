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
    // Go through the memory try to assign something then read it.
    // If the read value is what we've assigned before then we consider this a
    // valid memory block.
    volatile uintptr_t* addr = (uintptr_t*)&_kernel_end;
    size_t memory_size = 0;

    while (true) {
        ++addr;
        // This is why addr is volatile, if it weren't compiler would assume
        // that what we assigned must be there.
        *addr = (uintptr_t)addr;
        if (*addr != (uintptr_t)addr) {
            break;
        }
        memory_size += sizeof(addr);
    }
    return memory_size;
}
