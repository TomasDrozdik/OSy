// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _UTILS_H
#define _UTILS_H

/*
 * Declarations of help number functions.
 */

static inline int abs(int val) {
    return (val < 0) ? -val : val;
}

static inline size_t round_up(size_t value, size_t multiple_of) {
    size_t remainder;
    remainder = value % multiple_of;
    if (remainder == 0) {
        return value;
    }
    return value - remainder + multiple_of;
}

static inline size_t round_down(size_t value, size_t multiple_of) {
    size_t remainder;
    remainder = value % multiple_of;
    if (remainder == 0) {
        return value;
    }
    return value - remainder;
}

static inline void swap(uint32_t* a, uint32_t* b) {
    uint32_t tmp = *a;
    *a = *b;
    *b = tmp;
}

static inline void memcpy(uintptr_t dest, uintptr_t src, size_t n) {
    panic_if(abs((int32_t)dest - (int32_t)src) < (int32_t)n,
            "memcpy: overlapping memory areas");
    for (size_t i = 0; i < n; ++i) {
        ((uint8_t*)dest)[i] = ((uint8_t*)src)[i];
    }
}

#endif
