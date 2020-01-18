// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _MM_TLB_H
#define _MM_TLB_H

#include <proc/context.h>

#define TLB_ENTRY_COUNT 48

#define INVALID_ASID 0

void handle_tlb_refill(context_t* context);

void invalidate_tlb(uint8_t asid);

#endif
