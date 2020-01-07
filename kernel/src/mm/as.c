// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <exc.h>
#include <mm/as.h>
#include <mm/heap.h>
#include <mm/frame.h>
#include <utils.h>

/** Stack of free ASIDs. */
static size_t free_asid_stack_top;
static uint8_t free_asid_stack[ASID_COUNT];

/** Initializes support for address spaces.
 *
 * Called once at system boot
 */
void as_init(void) {
    bool enable = interrupts_disable();

    for (size_t i = 0; i < ASID_COUNT; ++i) {
        free_asid_stack[i] = i;
    }
    free_asid_stack_top = 0;

    interrupts_restore(enable);
}

static inline uint8_t get_next_asid() {
    // Work with static global data -> prevent races.
    bool enable = interrupts_disable();
    panic_if(free_asid_stack_top >= ASID_COUNT,
            "Kernel has run out of ASIDs.\n");
    uint8_t asid = free_asid_stack[free_asid_stack_top++];
    interrupts_restore(enable);
    return asid;
}

/** Create new address space.
 *
 * @param size Address space size, must be aligned to PAGE_SIZE.
 * @param flags Flags (unused).
 * @return New address space.
 * @retval NULL Out of memory.
 */
as_t* as_create(size_t size, unsigned int flags) {
    panic_if(size % PAGE_SIZE != 0, "AS size not alligned to PAGE_SIZE.\n");
    as_t* as = kmalloc(sizeof(as_t));
    if (!as) {
        return NULL;
    }
    as->size = size;
    as->reference_counter = 1;
    errno_t err = frame_alloc(as->size / PAGE_SIZE, &as->phys);
    if (err == ENOMEM) {
        return NULL;
    }
    panic_if(err != EOK, "Invalid errno.\n");

    // If everything went right i.e. no memory shortage we can assign asid.
    as->asid = get_next_asid();
    return as;
}

/** Get size of given address space (in bytes). */
size_t as_get_size(as_t* as) {
    return as->size;
}

/** Destroy given address space, freeing all the memory. */
void as_destroy(as_t* as) {
    panic_if(as->reference_counter == 0, "Invalid value of reference counter.\n");
    if (--as->reference_counter > 0) {
        return;
    }
    errno_t err = frame_free(as->size / PAGE_SIZE, as->phys);
    panic_if(err != EOK, "AS free frame caused errno %s", errno_as_str(err));

    panic_if(free_asid_stack_top == 0, "Invalid ASID stack state.\n");

    bool enable = interrupts_disable();
    free_asid_stack[--free_asid_stack_top] = as->asid;
    interrupts_restore(enable);

    kfree(as);
}

/** Get mapping between virtual pages and physical frames.
 *
 * @param as Address space to use.
 * @param virt Virtual address, aligned to page size.
 * @param phys Where to store physical frame address the page is mapped to.
 * @return Error code.
 * @retval EOK Mapping found.
 * @retval EINVAL Virtual address @virt is is not aligned to PAGE_SIZE.
 * @retval ENOENT Mapping does not exist.
 */
errno_t as_get_mapping(as_t* as, uintptr_t virt, uintptr_t* phys) {
    assert(INITIAL_VIRTUAL_ADDRESS % PAGE_SIZE == 0);
    if (virt % PAGE_SIZE != 0) {
        return EINVAL;
    }
    if (!(virt >= INITIAL_VIRTUAL_ADDRESS &&
          virt <= INITIAL_VIRTUAL_ADDRESS + as->size)) {
        dprintk("Virtual address %p not in [%p, %p]\n",
                virt, INITIAL_VIRTUAL_ADDRESS, INITIAL_VIRTUAL_ADDRESS + as->size);
        return ENOENT;
    }
    *phys = as->phys + (virt - INITIAL_VIRTUAL_ADDRESS);
    dprintk("Mapped virtual address %p to %p.\n", virt, *phys);
    return EOK;
}
