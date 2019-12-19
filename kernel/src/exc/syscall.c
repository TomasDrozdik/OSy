// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <drivers/timer.h>
#include <exc.h>
#include <lib/print.h>

void handle_syscall(context_t* context) {
    // Upon sucess, shift EPC by 4 to move to the next instruction
    // (unlike e.g. TLBL, we do not want to restart it).
    context->epc += 4;
}
