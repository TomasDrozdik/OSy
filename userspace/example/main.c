// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <stddef.h>
#include <stdio.h>

int main(void) {
    __asm__ volatile(".insn\n\n.word 0x29\n\n");
    puts("[APPL]: Hello from userspace!");

    return 0;
}
