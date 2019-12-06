// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <proc/context.h>
#include <drivers/cp0.h>

.text
.set noat
.set noreorder

.globl handle_exception_general_asm
.ent handle_exception_general_asm
handle_exception_general_asm:
    /*
     * We first save $epc, $cause, $badvaddr and $status to a fixed static
     * memory location. This location is in KUSEG and will be always
     * present.
     *
     * We then disable interrupts as another exception would mean we
     * have lost the CP0 registers.
     *
     * We then store the context on the stack and also copy the special
     * registers to it. Note that this could cause TLB refill exception
     * (hence we started by saving the special registers).
     *
     * Once the context is stored on the stack, we can re-enable interrupts
     * to make the handler re-entrant. For simplicity, we keep interrupts
     * disabled (re-enabling them would also mean a slight more complex
     * in the epilogue of this handler).
     *
     * We then jump to C code to actually handle the exception.
     *
     * After return from the C code, we restore the registers from the
     * stack and return. Again, accessing the stack could cause TLB refill
     * hence we do two-phase restore. We restore all registers but copy
     * CP0 registers to static area first. TLB refill here is okay as we
     * do not use kernel scratch registers. When restoring CP0 registers no
     * exception should occur hence we can use the scratch registers to
     * restore status and epc (hence re-enabling interrupts and jumping back).
     *
     * Note that the return can happen after a relatively long time as
     * timer interrupt may cause reschedule. Hence the context could be
     * switched several times before we return here.
     */

    // Store special registers to KSEG memory first.
    la $k1, 0x80000300
    SAVE_CP0_REGISTERS $k1 $k0

    // Disable interrupts.
    la $k1, ~(0x00000018 | 0x00000002 | 0x00000001)
    mfc0 $k0, $REG_CP0_STATUS
    and $k0, $k1
    mtc0 $k0, $REG_CP0_STATUS
    addiu $sp, -CONTEXT_SIZE

    // Save all registers on stack.
    SAVE_REGISTERS $sp
    la $t1, 0x80000300
    COPY_CP0_REGISTERS_TO_CONTEXT $t1 $sp $t0

    // Jump to C code.
    or $a0, $0, $sp
    jal handle_exception_general
    addiu $sp, -ABI_STACK_FRAME
    addiu $sp, ABI_STACK_FRAME

    // Restore all registers from stack.
    la $t1, 0x80000300
    COPY_CP0_REGISTERS_FROM_CONTEXT $sp $t1 $t0
    LOAD_REGISTERS $sp
    addiu $sp, CONTEXT_SIZE

    // Restore special registers from KSEG.
    la $k1, 0x80000300
    LOAD_CP0_REGISTERS $k1 $k0

    eret
.end handle_exception_general_asm
