// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _LIBC_STDIO_H
#define _LIBC_STDIO_H

#include <stddef.h>

/** Type for representing base of a number.*/
typedef uint32_t base_t;

/** Convert a string to a long integer
 * @param nptr Pointer to given number.
 * @param endptr Pointer to next non nuber character.
 * @returns Parsed number
 */
long int strtol(const char* nptr, char** endptr);

/** Capitalizes given character.
 * Converts a-z to A-Z, other characters remain.
 * @param c Char to capitalize.unit32_i
 * @return Capitalized character
 */
int toupper(int c);

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

int putchar(int c);
int fputs(const char* s);
int puts(const char* s);
int printf(const char* format, ...);

#endif
