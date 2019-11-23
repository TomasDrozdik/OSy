// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug/mm.h>
#include <main.h>
#include <mm/heap.h>

/** Indicates if heap_init has been called. */
static bool heap_initialized = false;

/** Poiter to the start of the available memory. */
static uintptr_t start_ptr;

/** Poiter to the end of the available memory. */
static uintptr_t end_ptr;

/** Poiter to the start of the unallocated memory. */
static uintptr_t bump_ptr;

/** Initialized heap.
 * Uses _kernel_end to locate end of the kernel then function
 * debug_get_base_memory_endptr to get the pointer to the end of the continuous
 * address space following end of kernel.
 *
 * Initialized globals @start_ptr, @end_ptr and @bump_ptr.
 */
static void heap_init(void);

/** Align the pointer.
 * @param ptr Pointer to align.
 * @param size Alignt according to the given size.
 * @returns Aligned pointer.
 */
static inline uintptr_t align(uintptr_t ptr, size_t size);

void* kmalloc(size_t size) {
    // TODO: this is just simple bump pointer implementation described in
    // assignment description. Update this to more sophisticated one.
    if (!heap_initialized) {
        heap_init();
        heap_initialized = true;
    }
    uintptr_t ptr = bump_ptr;
    // Check if there is still available memory.
    if (ptr >= end_ptr) {
        return NULL;
    }
    // Increase global start of unallocated memory.
    bump_ptr = align(bump_ptr + size, 4);
    return (void*)ptr;
}

void kfree(void* ptr) {
    // TODO:
}

static void heap_init(void) {
    start_ptr = align((uintptr_t)&_kernel_end, 4);
    end_ptr = debug_get_base_memory_endptr();
    bump_ptr = start_ptr;
}

static inline uintptr_t align(uintptr_t ptr, size_t size) {
    size_t remainder;

    remainder = ptr % size;
    if (remainder == 0) {
        return ptr;
    }

    return ptr - remainder + size;
}


