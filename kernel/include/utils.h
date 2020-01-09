// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _UTILS_H
#define _UTILS_H

/*
 * Declarations of help number functions.
 */

inline size_t round_up(size_t value, size_t multiple_of) {
    size_t remainder;
    remainder = value % multiple_of;
    if (remainder == 0) {
        return value;
    }
    return value - remainder + multiple_of;
}

inline size_t round_down(size_t value, size_t multiple_of) {
    size_t remainder;
    remainder = value % multiple_of;
    if (remainder == 0) {
        return value;
    }
    return value - remainder;
}

inline void swap(uint32_t* a, uint32_t* b) {
    uint32_t tmp = *a;
    *a = *b;
    *b = tmp;
}

#endif
