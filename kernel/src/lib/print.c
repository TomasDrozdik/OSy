// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/list.h>
#include <debug.h>
#include <drivers/printer.h>
#include <lib/print.h>
#include <lib/stdarg.h>
#include <types.h>

/** Prints given formatted string to console.
 *
 * @param format printf-style formatting string.
 */
void printk(const char* format, ...) {
    const char* it = format;
    int temp;
    list_t* list;
    char* str;
    bool first = true;

    va_list argp;
    va_start(argp, format);
    while (*it != '\0') {
        if (*it == '%') {
            it++;
            if (*it == '%') {
                printer_putchar('%');
            } else {
                switch (*it) {
                case 'c':
                    temp = va_arg(argp, int);
                    printer_putchar(temp);
                    break;
                case 'u':
                    temp = va_arg(argp, unsigned int);
                    fputs(convert(temp, 10, 0, false));
                    break;
                case 'x':
                    temp = va_arg(argp, unsigned int);
                    it++;
                    if (*it == 'f') {
                        fputs(convert(temp, 16, 0, true));
                    } else {
                        fputs(convert(temp, 16, 0, false));
                        printer_putchar(*it);
                    }
                    break;
                case 'X':
                    temp = va_arg(argp, unsigned int);
                    it++;
                    if (*it =='f'){
						fputs(convert(temp, 16, 1,true));
                    } else {
                        fputs(convert(temp, 16, 1, false));
                        printer_putchar(*it);
					}
                    break;
                case 'd':
                    temp = va_arg(argp, int);
                    if (temp < 0) {
                        temp = -temp;
                        printer_putchar('-');
                    }
                    fputs(convert(temp, 10, 0,false));
                    break;
                case 's':
                    str = va_arg(argp, char*);
                    fputs(str);
                    break;
                case 'p':
                    temp = va_arg(argp, unsigned int);
                    fputs("0x");
                    fputs(convert(temp, 16, 0,false));
                    it++;
                    if (*it == 'L') {
                        list =(list_t*) temp;
                        printer_putchar('[');
                        if (list_is_empty(list)) {
                            fputs("empty]");
						} else {
                            fputs(convert(list_get_size(list),10,0,false));
                            fputs(": ");
							list_foreach(*list, list_t, head, iter) {
                                if (!first) {
									printer_putchar('-');
								}
                                fputs("0x");
                                fputs(convert((int)iter, 16, 0,false));
                                first = false;
							}
                            printer_putchar(']');
                            first = true;
						}
					} else {
                        printer_putchar(*it);
					}
                    break;
                }
            }
        } else {
            if (*it == '\n') {
                fputs("\r\n");
            } else {
                printer_putchar(*it);
            }
        }
        it++;
    }
}

char* convert(unsigned int n, int base, int cas,_Bool format) {
    static char repre[17] = "0123456789abcdef";
    static char buffer[50];
    char* ptr;

    if (cas == 1) {
        for (int i = 10; i < 17; i+=1) {
            repre[i] = repre[i] - 32;
        }
    }

    ptr = &buffer[49];
    *ptr = '\0';
    int count = 0;

    do {
        *--ptr = repre[n % base];
        count += 1;
        n /= base;
    } while (n != 0);

	if (format) {
        while (count < 8) {
            *--ptr = '0';
            count += 1;
        }
        count = 0;
    }

	if (cas == 1) {    
        for (int i = 10; i < 17; i += 1) {
            repre[i] = repre[i] + 32;
        }
    }

    return (ptr);
}

/** Prints given string to console,without terminating with newline.
 *
 * @param s String to print.
 */
void fputs(const char* s) {
    while (*s != '\0') {
        printer_putchar(*s);
        s++;
    }
}

/** Prints given string to console, terminating it with newline.
 *
 * @param s String to print.
 */
void puts(const char* s) {
    while (*s != '\0') {
        printer_putchar(*s);
        s++;
    }
    printer_putchar('\n');
}
