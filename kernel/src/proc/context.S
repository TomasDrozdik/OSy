// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

.text
.set noat
.set noreorder

#include <proc/context.h>

#define entryhi 10
#define status 12

/*
 * cpu_switch_context
 *
 * Switches processor context from one thread to another. The first argument
 * points to a pointer to the top of the stack of the old thread, the
 * second argument points to a pointer to the top of the stack of
 * the new thread.
 *
 * The pointer to the old stack is written, the pointer to the new stack
 * is read. Both are passed as pointers to handle the weird case where
 * the old thread and the new thread are identical, because then,
 * we would not know the address of the new stack before it
 * is written as the address of the old stack.
 *
 * The third argument contains the address space identifier of the thread
 * we are switching to. The ASID value is set to the EntryHi register after
 * the context of the old thread is saved, but before the context of the
 * new thread is loaded.
 */

.globl cpu_switch_context
.ent   cpu_switch_context

cpu_switch_context:

    /*
     * Allocate a frame on the stack of the old thread and update
     * the address of the stack top of the old thread.
     */

    addiu $sp, -CONTEXT_SIZE
    sw $sp, ($a0)

    /*
     * Store the general registers and the $hi, $lo and Status
     * registers on the stack. Disable interrupts after
     * storing the Status register.
     */

    SAVE_REGISTERS $sp

    mfc0 $t0, $status
    sw $t0, 152($sp)
    la $t1, ~0x00000001
    and $t0, $t1
    mtc0 $t0, $status

    /*
     * Switch to the address space of the new thread.
     */

    mfc0 $t0, $entryhi
    la $t1, ~0x000000ff
    and $t0, $t1
    or $t0, $a2
    mtc0 $t0, $entryhi

    /*
     * Switch to the stack of the new thread. The stack of the
     * new thread should be in the same state as the stack of
     * the old thread.
     */

    lw $sp, ($a1)

    /* Load the general registers and the $hi and $lo registers from the stack. */

    LOAD_REGISTERS $sp

    /*
     * Prepare to restore the Status register. The Status register is
     * restored in the branch delay slot of the jump that returns
     * control to the new thread.
     *
     * Setting the Status register in the branch delay slot makes it
     * possible to return from kernel mode to user mode. Setting the
     * register sooner would mean switching from kernel mode to user
     * mode while executing in KSEG0, which is not allowed.
     *
     * A somewhat cleaner alternative to this particular method of
     * returning from kernel mode to user mode is the ERET instruction.
     */

    lw $k0, 152($sp)
    addiu $sp, CONTEXT_SIZE

    /*
     * Note that from now on, an interrupt or an exception here would
     * trash the content of the $k0 and $k1 registers. This is why we
     * have disabled interrupts earlier, and do not touch anything
     * in memory.
     */

    j $ra
    mtc0 $k0, $status

.end cpu_switch_context
