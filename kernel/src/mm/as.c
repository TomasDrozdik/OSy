// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <mm/as.h>

/** Initializes support for address spaces.
 *
 * Called once at system boot
 */
void as_init(void) {
}

/** Create new address space.
 *
 * @param size Address space size, must be aligned to PAGE_SIZE.
 * @param flags Flags (unused).
 * @return New address space.
 * @retval NULL Out of memory.
 */
as_t* as_create(size_t size, unsigned int flags) {
    return NULL;
}

/** Get size of given address space (in bytes). */
size_t as_get_size(as_t* as) {
    return 0;
}

/** Destroy given address space, freeing all the memory. */
void as_destroy(as_t* as) {
}

/** Get mapping between virtual pages and physical frames.
 *
 * @param as Address space to use.
 * @param virt Virtual address, aligned to page size.
 * @param phys Where to store physical frame address the page is mapped to.
 * @return Error code.
 * @retval EOK Mapping found.
 * @retval ENOENT Mapping does not exist.
 */
errno_t as_get_mapping(as_t* as, uintptr_t virt, uintptr_t* phys) {
    return ENOIMPL;
}
