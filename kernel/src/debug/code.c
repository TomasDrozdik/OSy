// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug/code.h>
#include <lib/print.h>

/** Dump function code at given address.
 *
 * Generally, the output should look like disassembly without
 * mnemonics part.
 *
 * @param name Function name to print in header.
 * @param address Function address.
 * @instruction_count How many instructions to print.
 */
void debug_dump_function(const char* name, uintptr_t address, size_t instruction_count) {
	printk("%xf <%s>:\n", address, name);
    uintptr_t* addr;
    for (size_t i = 0; i < instruction_count; i+=1) {
        addr = (uintptr_t*)address;
        printk("%xf:        %xf\n", addr, *addr);
        address = address + 0x00000004;
	}
}
