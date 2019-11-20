// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#ifndef _PROC_CONTEXT_H
#define _PROC_CONTEXT_H

/*
 * This file is shared (as we want to keep the offsets etc. together)
 * across C sources and pure assembler sources so we use ifdefs to include
 * only relevant portions of the code.
 */

/** Size of context_t structure. */
#define CONTEXT_SIZE 156

/** Minimal stack frame size according to MIPS o32 ABI. */
#define ABI_STACK_FRAME 32

#ifndef __ASSEMBLER__

#include <types.h>

/** CPU context (registers). */
typedef struct {
    unative_t zero;

    unative_t at;
    unative_t v0;
    unative_t v1;

    unative_t a0;
    unative_t a1;
    unative_t a2;
    unative_t a3;

    unative_t t0;
    unative_t t1;
    unative_t t2;
    unative_t t3;
    unative_t t4;
    unative_t t5;
    unative_t t6;
    unative_t t7;
    unative_t t8;
    unative_t t9;

    unative_t s0;
    unative_t s1;
    unative_t s2;
    unative_t s3;
    unative_t s4;
    unative_t s5;
    unative_t s6;
    unative_t s7;

    unative_t k0;
    unative_t k1;

    unative_t gp;
    unative_t fp;

    unative_t sp;
    unative_t ra;

    unative_t lo;
    unative_t hi;

    unative_t epc;
    unative_t cause;
    unative_t badva;
    unative_t entryhi;
    unative_t status;
} context_t;

void cpu_switch_context(void** stack_top_old, void** stack_top_new, asid_t asid_new);

#endif

#ifdef __ASSEMBLER__
// clang-format off
.macro SAVE_REGISTERS base
    sw $zero, 0(\base)

    sw $at, 4(\base)

    sw $v0, 8(\base)
    sw $v1, 12(\base)

    sw $a0, 16(\base)
    sw $a1, 20(\base)
    sw $a2, 24(\base)
    sw $a3, 28(\base)

    sw $t0, 32(\base)
    sw $t1, 36(\base)
    sw $t2, 40(\base)
    sw $t3, 44(\base)
    sw $t4, 48(\base)
    sw $t5, 52(\base)
    sw $t6, 56(\base)
    sw $t7, 60(\base)
    sw $t8, 64(\base)
    sw $t9, 68(\base)

    sw $s0, 72(\base)
    sw $s1, 76(\base)
    sw $s2, 80(\base)
    sw $s3, 84(\base)
    sw $s4, 88(\base)
    sw $s5, 92(\base)
    sw $s6, 96(\base)
    sw $s7, 100(\base)

    sw $gp, 112(\base)
    sw $fp, 116(\base)
    sw $ra, 124(\base)
.endm SAVE_REGISTERS

.macro LOAD_REGISTERS base
    lw $ra, 124(\base)
    lw $fp, 116(\base)
    lw $gp, 112(\base)

    lw $s7, 100(\base)
    lw $s6, 96(\base)
    lw $s5, 92(\base)
    lw $s4, 88(\base)
    lw $s3, 84(\base)
    lw $s2, 80(\base)
    lw $s1, 76(\base)
    lw $s0, 72(\base)

    lw $t9, 68(\base)
    lw $t8, 64(\base)
    lw $t7, 60(\base)
    lw $t6, 56(\base)
    lw $t5, 52(\base)
    lw $t4, 48(\base)
    lw $t3, 44(\base)
    lw $t2, 40(\base)
    lw $t1, 36(\base)
    lw $t0, 32(\base)

    lw $a3, 28(\base)
    lw $a2, 24(\base)
    lw $a1, 20(\base)
    lw $a0, 16(\base)

    lw $v1, 12(\base)
    lw $v0, 8(\base)

    lw $at, 4(\base)

    lw $zero, 0(\base)
.endm LOAD_REGISTERS
// clang-format on
#endif

#endif
