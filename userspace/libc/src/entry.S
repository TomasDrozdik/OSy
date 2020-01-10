// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Userspace binary bootstrap code.
 * This code will be placed at a fixed (virtual) address 0x4000.
 *
 * We place it in a special section so we can reference it from a linker
 * script.
 */

// ABI requirement for initial free space on top of the stack.
#define ABI_STACK_FRAME 32

.set noreorder

.section .entry, "ax"

.org 0x4000
.global __entry
.ent __entry
__entry:
	.frame $sp, ABI_STACK_FRAME, $ra
	.cpload $t9
	addiu $sp, -ABI_STACK_FRAME
	.cprestore
	
	jal __main
	nop
.end __entry
