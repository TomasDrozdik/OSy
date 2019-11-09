// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _LIB_PRINT_H
#define _LIB_PRINT_H

void printk(const char* format, ...);
char* convert(unsigned int n, int base, int cas, _Bool format);
void fputs(const char* s);
void puts(const char* s);

#endif
