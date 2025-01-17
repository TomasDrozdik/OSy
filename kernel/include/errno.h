// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _ERRNO_H
#define _ERRNO_H

#include <types.h>

#define EOK 0
#define ENOIMPL 1
#define ENOMEM 2
#define EBUSY 3
#define EEXITED 4
#define EINVAL 5

typedef int errno_t;

static inline const char* errno_as_str(errno_t err) {
    switch (err) {
    case EOK:
        return "no error";
    case ENOIMPL:
        return "not implemented";
    case ENOMEM:
        return "out of memory";
    case EBUSY:
        return "resource is busy";
    case EEXITED:
        return "thread already exited";
    case EINVAL:
        return "invalid value";
    default:
        return "unknown error";
    }
}

#endif
