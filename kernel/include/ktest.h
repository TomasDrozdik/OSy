// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _KTEST_H
#define _KTEST_H

#include <debug.h>
#include <errno.h>
#include <lib/print.h>

/*
 * Prefiexes for test output, used later by check_output.py script.
 */
#define KTEST_EXPECTED "[EXPECTED]: "
#define KTEST_BLOCK_EXPECTED "[EXPECTED BLOCK]: "
#define KTEST_ACTUAL "[ ACTUAL ]: "

/** Print header of a kernel test. */
#define ktest_start(name) \
    puts("== KERNEL TEST " name " ==")

/** Print message about passed kernel test. */
#define ktest_passed() \
    puts("\n\nTest passed.\n\n")

/** Print message about passed failed test and halts the CPU. */
#define ktest_failed() \
    do { \
        puts("\n\nTest failed.\n\n"); \
        machine_halt(); \
    } while (0)

/** Kernel test assertion.
 *
 * Unlike normal assertion, this one is always checked and machine is
 * terminated when expr does not evaluate to true.
 */
#define ktest_assert(expr, fmt, ...) \
    do { \
        if (!(expr)) { \
            puts("\n\n" __FILE__ ":" QUOTE_ME(__LINE__) ": Kernel test assertion failed: " #expr); \
            printk(__FILE__ ":" QUOTE_ME(__LINE__) ": " fmt "\n", ##__VA_ARGS__); \
            ktest_failed(); \
        } \
    } while (0)

/** Kernel test assertion for bound checking. */
#define ktest_assert_in_range(msg, value, lower, upper) \
    ktest_assert(((value) >= (lower)) && ((value) <= (upper)), \
            #value "=%d not in [" #lower "=%d, " #upper "=%d] range", \
            value, lower, upper)

#define ktest_assert_errno(value, last_operation_name) \
    ktest_assert((value) == EOK, "%s unexpectedly failed with %d (%s)", \
            last_operation_name, (value), errno_as_str(value))

/** All kernel test share this signature as only one test is compiled at a time. */
void kernel_test(void);

#endif
