// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/types.h>
#include <np/utest.h>
#include <stddef.h>
#include <stdio.h>

/*
 * Checks that using control coprocessor causes forced process termination.
 */

int main(void) {
    utest_start("basic/cp0");
    utest_expect_abort();

    __asm__ volatile("mtc0 $a0, $12\n");

    printf("Survived access to control coprocessor.\n");

    utest_failed();

    return 0;
}
