// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <stddef.h>
#include <stdio.h>

int main(void) {
    utest_start("printf/uint");

    puts("[APPL]: Hello from userspace!");

    //__asm__ volatile("mtc0 $a0, $12\n");

    return 0;
}
