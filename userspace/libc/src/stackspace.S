// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

/*
 * Dummy section to provide space for stack and NULL page.
 */

.set noreorder

.section .pre_entry_space, "ax"

.org 0x0
.global __pre_entry_space
.ent __pre_entry_space
__pre_entry_space:
	nop
.end __pre_entry_space
