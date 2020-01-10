// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/syscall.h>
#include <stdio.h>

/** Print single character to console.
 *
 * @param c Character to print.
 * @return Character written as an unsigned char cast to an int.
 */
int putchar(int c) {
    return (unsigned char)c;
}

/** Prints given string to console, terminating it with newline.
 *
 * @param s String to print.
 * @return Number of printed characters.
 */
int puts(const char* s) {
    return -1;
}

/** Prints given formatted string to console.
 *
 * @param format printf-style formatting string.
 * @return Number of printed characters.
 */
int printf(const char* format, ...) {
    return -1;
}
