// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Kernel bootstrap code.
 *
 * This code will be loaded at (physical) address 0.
 */

#include <drivers/cp0.h>

/*
 * MSIM-specific macro for halting the simulator.
 */
.macro halt
    .insn
    .word 0x28

    /*
     * Loop forever in case the above instruction is not
     * properly recognized by the simulator.
     * As a matter of fact, if the instruction is not recognized,
     * exception should have been raised but we are a bit paranoid
     * here ;-).
     */

    0:
        j 0b
        nop

.endm halt

/*
 * MSIM-specific macro for entering interactive mode.
 */
.macro msim_stop
    .insn
    .word 0x29
.endm msim_stop

/*
 * For now, dump registers and enter interactive mode on any exception.
 */
.macro announce_exception
    .insn
    .word 0x37
    msim_stop
.endm announce_exception

/*
 * Switch to normal exception vector (stop using BEV).
 *
 * Note: BEV bit is 22, status register is 12.
 *
 * Note: we know when this will be called so we can safely use $0 and $1.
 */
.macro use_normal_exception_vector
    la $k1, ~(1 << 22)
    mfc0 $k0, $REG_CP0_STATUS
    and $k0, $k0, $k1
    mtc0 $k0, $REG_CP0_STATUS
    nop
.endm use_normal_exception_vector



/*
 * We know what whare are doing here so do not let
 * assembler change our code in any way.
 */

.set noat
.set noreorder

/*
 * Put the hardwired code to a special input section ".excvec".
 * This section is then placed by the linker (as instructed by the kernel
 * linker script) at the very beginning of the kernel image (address 0).
 */

.section .excvec, "ax"

.globl handle_exception_general_asm

/*
 * Debugging-only exception handling: enter interactive mode.
 */
.org   0x000
.globl exception_tlb_refill
.ent   exception_tlb_refill
exception_tlb_refill:
    j handle_tlb_refill_asm
    nop
.end exception_tlb_refill

.org   0x100
.globl exception_cache_error
.ent   exception_cache_error
exception_cache_error:
    announce_exception
.end exception_cache_error

.org   0x180
.globl exception_general
.ent   exception_general
exception_general:
    j handle_exception_general_asm
    nop
.end exception_general

/*
 * Kernel entry point.
 * The bootstrap loader (0x1FC00000) jumps here.
 * We only jump to the C code from here.
 */

.org   0x400
.globl start
.ent   start

start:
    la $sp, 0x80000400
    la $gp, 0x80000000
    use_normal_exception_vector
    jal kernel_main
    nop

    /*
     * Again, paranoid style: this is unreachable anyway.
     */
    halt
.end start

.org 0x1000
.globl tlb_refill_stack_space
.ent tlb_refill_stack_space

tlb_refill_stack_space:
    nop
.end tlb_refill_stack_space

/*
 * Rest of code.
 */
.text

