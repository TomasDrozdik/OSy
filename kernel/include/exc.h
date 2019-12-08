// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _EXC_H
#define _EXC_H

#include <proc/context.h>

void handle_exception_general(context_t* context);
bool interrupts_disable(void);
void interrupts_restore(bool enable);

#endif
