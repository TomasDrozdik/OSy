// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _LIB_PRINT_H
#define _LIB_PRINT_H

#include <types.h>
#include <drivers/printer.h>

/** Type for representing base of a number.*/
typedef uint32_t base_t;

/** Wrapper around printer putchar. */
static inline void putchar(char c) {
    printer_putchar(c);
}

/** Prints given string to console.
 * @param s String to print.
 */
int fputs(const char* s);

/** Prints given string to console, terminating it with newline.
 * @param s String to print.
 */
int puts(const char* s);

/** Prints given formatted string to console.
 * Supported printf formats: %c, %d, %u, %s, %x, %X, %p, %pL
 * @param format printf-style formatting string.
 */
void printk(const char* format, ...);

/**
 * TODO
 */
char* convert(unsigned int n, int base, int cas, _Bool format);

/** Capitalizes given character.
 * Converts a-z to A-Z, other characters remain.
 * @param c Char to capitalize.unit32_i
 * @return Capitalized character
 */
int toupper(int c);

/** Convert a string to a long integer
 * @param nptr Pointer to given number.
 * @param endptr Pointer to next non nuber character.
 * @returns Parsed number
 */
long int strtol(const char* nptr, char** endptr);

/** Convert int32_t to string given fixed sized buffer.
 * If buffer is not big enoght to hold the number in given base then return -1.
 * @param n Number to print.
 * @param base Print the number w.r.t. this base.
 * @param buf Pointer buffer.
 * @param buflen Size of the buffer.
 * @returns Length of the string converted OR -1 on small buffer.
 */
int int32_to_str(int32_t n, base_t base, char* buf, size_t buflen);

/** Convert uint32_t to string given fixed sized buffer.
 * If buffer is not big enoght to hold the number in given base then return -1.
 * @param n Number to print.
 * @param base Print the number w.r.t. this base.
 * @param buf Pointer buffer.
 * @param buflen Size of the buffer.
 * @returns Length of the string converted OR -1 on small buffer.
 */
int uint32_to_str(uint32_t n, base_t base, char* buf, size_t buflen);

char* strncpy(char* dest, const char* src, size_t n);

#endif
