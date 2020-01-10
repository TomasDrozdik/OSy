// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

.text
.set noat
.set noreorder

#include <proc/context.h>

#define status 12
#define epc 14

/*
 * cpu_jump_to_userspace
 *
 * Switches processor to userspace code. The first argument is address
 * of the top of userland stack, the second argument is the address where
 * to continue the execution (i.e. userspace entry point).
 *
 * The jump is done via setting status register to emulate exception handling,
 * setting EPC and stack pointer and then calling ERET that jumps back
 * (actually forward ;-) to userspace.
 */

.globl cpu_jump_to_userspace
.ent   cpu_jump_to_userspace

cpu_jump_to_userspace:
    move $sp, $a0
    mtc0 $a1, $epc
    la $a0, (0x1 | (0x1 << 1) | (0x2 << 3))
    mtc0 $a0, $status
    eret
    nop
.end cpu_jump_to_userspace
