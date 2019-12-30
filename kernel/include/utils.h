// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _UTILS_H
#define _UTILS_H

/*
 * Declarations of help number funcitons.
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

#endif
