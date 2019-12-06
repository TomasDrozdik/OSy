// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/list.h>
#include <debug.h>
#include <drivers/printer.h>
#include <lib/print.h>
#include <lib/stdarg.h>
#include <proc/thread.h>

#define BUFFER_SIZE 20

/** Shifts string to the right.
 *
 * WARNING: Does not check boundries of the string!
 *
 * @param strstart Start of the string.
 * @param strend End of the string, pointer to one char past the last char.
 * @param shiftsize Size of the shift.
 */
static inline void shift_str_right(char* strstart, char* strend,
        size_t shiftsize);

/** Fills given string with given char.
 *
 * WARNING: Does not check boundries of the string!
 *
 * @param strstart Pointer to the begining of the string to fill.
 * @param strend Pointer past the end of the string to fill.
 * @param fillchar Character to fill the string with.
 */
static inline void fill(char* strstart, char* strend, char fillchar);

/** Get the order of the number i.e. number of digits according to given base.
 * @param n Number to process.
 * @param base Compute order according to this base.
 */
static inline size_t get_order(uint32_t n, base_t base);

/** Print given integer.
 * In case of buffer overflow assert fails.
 * @param n Number to print.
 * @param is_signed Is the number signed?
 * @param base Base in which the number should be printed.
 * @param has_width Is width specifier set?
 * @param min_width Minimal width specifier?
 * @param capitalize Should the whole string be run through to_upper funciton?
 * @param buf Fixed size buffer to store the string in.
 */
static void print_integer(uint32_t n, bool is_signed, base_t base,
        bool has_width, int min_width, bool capitalize, char buf[BUFFER_SIZE]);

/** Prints the pointer.
 * Format: 0x<hexa_value_of_pointer>
 * In case of buffer overflow assert fails.
 * @param p Pointer value to print.
 * @param buf Fixed size buffer to store the string in.
 */
static void print_pointer(void* p, char buf[BUFFER_SIZE]);

/** Prints list
 * Format: for empty: "[empty]"
 *         non empty: "[#item_count: 0th-link_t_address-1st-link_t_address...]
 * @param list List to print.
 * @param buf Fixed size buffer to store individual poitners and numbers.
 *            Buffer stores only single pointer / number at once. It doesn't
 *            store whole list. Use similar buffer as in print_pointer or
 *            print_integer.
 */
static void print_list(list_t* list, char buf[BUFFER_SIZE]);

/** Prints thread info
 * Format: // TODO
 * @param list List to print.
 */
static void print_thread(thread_t* thread);

/** Implementation of uint32_t_to_string.
 * Converts uint32_t number n with given order and base to string stored in dst
 * which does have sufficient memory allocated w.r.t. order.
 * Order has to be valid w.r.t. chosen base.
 * @param n number to convert
 * @param buf pointer to having enought alocated memory
 * @param order of n w.r.t. base
 * @param base of the converion
 */
static void uint32_to_str_impl(uint32_t n, char* buf, int order,
        int base);

void fputs(const char* s) {
    while (*s != '\0') {
        printer_putchar(*s);
        s++;
    }
}

void puts(const char* s) {
    fputs(s);
    printer_putchar('\n');
}

void printk(const char* format, ...) {
    va_list args;
    va_start(args, format);

    char* endptr;
    bool has_width = false;
    unsigned min_width = 0;

    base_t base;
    bool is_signed = false, capitalize = false;

    char buf[BUFFER_SIZE];

    for (const char* cp = format; *cp != '\0'; ++cp) {
        if (*cp != '%') {
            printer_putchar(*cp);
            continue;
        }
        if (*(++cp) == '\0') {
            assert(false);
        }

        // Check for length specifier.
        min_width = strtol(cp, &endptr);
        has_width = (cp != endptr);
        cp = endptr;

        switch (*cp) {
        case 'c':
            // Use int instead of char due to promotion.
            printer_putchar(va_arg(args, int));
            break;
        case 'd':
            is_signed = true;
            base = 10;
            capitalize = false;
            print_integer((uint32_t)va_arg(args, int32_t), is_signed, base,
                    has_width, min_width, capitalize, buf);
            break;
        case 'p':
            switch (*(++cp)) {
            case 'L':
                print_list(va_arg(args, list_t*), buf);
                break;
            case 'T':
                print_thread(va_arg(args, thread_t*));
                break;
            default:
                --cp;
                print_pointer(va_arg(args, void*), buf);
            }
            break;
        case 's':
            fputs(va_arg(args, const char*));
            break;
        case 'u':
            is_signed = false;
            base = 10;
            capitalize = false;
            print_integer(va_arg(args, uint32_t), is_signed, base, has_width,
                    min_width, capitalize, buf);
            break;
        case 'X':
            is_signed = false;
            base = 16;
            capitalize = true;
            print_integer(va_arg(args, uint32_t), is_signed, base, has_width,
                    min_width, capitalize, buf);
            break;
        case 'x':
            is_signed = false;
            base = 16;
            capitalize = false;
            print_integer(va_arg(args, uint32_t), is_signed, base, has_width,
                    min_width, capitalize, buf);
            break;
        case '%':
            printer_putchar('%');
            break;
        default:
            assert(false);
        }
    }
    va_end(args);
}

int toupper(int c) {
    if (c >= 'a' && c <= 'z') {
        c -= 'a' - 'A';
    }
    return c;
}

int uint32_to_str(uint32_t n, base_t base, char* buf, size_t buflen) {
    const size_t order = get_order(n, base);
    if (order > buflen) {
        return -1;
    }
    uint32_to_str_impl(n, buf, order, base);
    return order;
}

int int32_to_str(int32_t n, base_t base, char* buf, size_t buflen) {
    bool is_negative = false;
    if (n < 0) {
        is_negative = true;
        n *= (-1); // make n possitive
    }

    const size_t order = get_order(n, base);
    if (order + is_negative > buflen) {
        return -1;
    }
    if (is_negative) {
        buf[0] = '-';
    }
    uint32_to_str_impl((uint32_t)n, buf + is_negative, order, base);
    return order + is_negative;
}

long int strtol(const char* nptr, char** endptr) {
    *endptr = (char*)nptr;
    long int i = 0;
    bool is_negative = false;
    if ((is_negative = **endptr == '-') || **endptr == '+') {
        ++endptr;
    }
    while (**endptr >= '0' && **endptr <= '9') {
        i = i * 10 + **endptr - '0';
        ++*endptr;
    }
    return i;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; ++i) {
        dest[i] = src[i];
    }
    for ( ; i < n; ++i) {
        dest[i] = '\0';
    }
    return dest;
}

static inline void shift_str_right(char* strstart, char* strend,
        size_t shiftsize) {
    for (char* cp = strend - 1; cp >= strstart; --cp) {
        *(cp + shiftsize) = *cp;
    }
}

static inline void fill(char* strstart, char* strend, char fillchar) {
    for (char* cp = strstart; cp < strend; ++cp) {
        *cp = fillchar;
    }
}

static inline size_t get_order(uint32_t n, base_t base) {
    size_t order = 0;
    do {
        ++order;
        n /= base;
    } while (n != 0);
    return order;
}

static void print_integer(uint32_t n, bool is_signed, base_t base,
        bool has_width, int min_width, bool capitalize, char buf[BUFFER_SIZE]) {
    if (min_width > BUFFER_SIZE) {
        assert(false);
    }

    int width;
    if (is_signed) {
        width = int32_to_str(n, base, buf, BUFFER_SIZE);
    } else {
        width = uint32_to_str(n, base, buf, BUFFER_SIZE);
    }
    if (width == -1) {
        assert(false);
    }

    if (has_width && width < min_width) {
        char* one_past_end = buf + width + 1;
        size_t shift_size = min_width - width;
        shift_str_right(buf, one_past_end, shift_size);

        one_past_end = buf + shift_size;
        fill(buf, one_past_end, '0');
    }

    if (capitalize) {
        for (char* cp = buf; *cp != '\0'; ++cp) {
            *cp = toupper(*cp);
        }
    }
    fputs(buf);
}

static void print_pointer(void* p, char buf[BUFFER_SIZE]) {
    fputs("0x");
    bool is_signed = false, has_width = false, capitalize = false;
    base_t base = 16;
    size_t min_width = 0;
    print_integer((uintptr_t)p, is_signed, base, has_width, min_width,
            capitalize, buf);
}

static void print_list(list_t* list, char buf[BUFFER_SIZE]) {
    const size_t size = list_get_size(list);
    if (size == 0) {
        print_pointer(list, buf);
        fputs("[empty]");
        return;
    }

    link_t* it = list->head.next;
    printk("%p[%u: %p", list, size, it);

    if (size > 1) {
        for (it = it->next; it != &list->head; it = it->next) {
            printer_putchar('-');
            print_pointer(it, buf);
        }
    }
    printer_putchar(']');
}

static void print_thread(thread_t* thread) {
    printk("Thread[%p] %s:"
           "\tstate: %s"
           "\tentry_func: %p"
           "\tdata: %p"
           "\tstack_top %p"
           "\tcontext: %p"
           "\tsp: %p"
           "\tra: %p",
           thread,
           thread->name,
           (thread->state == READY) ? "READY" :
                   (thread->state == SUSPENDED) ? "SUSPENDED" : "FINISHED",
           thread->entry_func,
           thread->data,
           thread->stack_top,
           THREAD_INITIAL_CONTEXT(thread),
           THREAD_INITIAL_CONTEXT(thread)->sp,
           THREAD_INITIAL_CONTEXT(thread)->ra);
}

static void uint32_to_str_impl(uint32_t n, char* buf, int order,
        int base) {
    buf[order] = '\0';

    // Represent uint in string first using digits '0'-'9' then alphabet 'a'-...
    // according to selected base.
    // Base should be reasonable s.t. it fits alphabet otherwise we start
    // printing characters following 'z' whatever that is :D
    for (int i = order - 1; i >= 0; --i) {
        char digit = n % base;
        buf[i] = (digit < 10) ? digit + '0' : digit - 10 + 'a';
        n /= base;
    }
}
