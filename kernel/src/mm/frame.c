// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/bitmap.h>
#include <debug/mm.h>
#include <exc.h>
#include <lib/print.h>
#include <mm/frame.h>
#include <types.h>
#include <utils.h>

#define KSEG0_BASE 0x80000000

#define KSEG0_TO_PHYS(kseg0_addr) \
    ((kseg0_addr)-KSEG0_BASE)

#define PHYS_TO_KSEG0(phys_addr) \
    ((phys_addr) + KSEG0_BASE)

#define GET_ADDRESS(idx) \
    (page_start + (idx)*FRAME_SIZE)

#define GET_INDEX(address) \
    (((address)-page_start) / FRAME_SIZE)

static bitmap_t bitmap;
static uint8_t* backing_field;
static size_t page_count;

static uintptr_t start;
static uintptr_t page_start;
static uintptr_t end;

/**
 * Initializes frame allocator.
 *
 * Called once at system boot.
 */
void frame_init(void) {
    bool enable = interrupts_disable();

    // Find start and end of address space in kseg0 alligned to FRAME_SIZE.
    start = round_up(debug_get_kernel_endptr(), FRAME_SIZE);
    end = round_down(debug_get_base_memory_endptr(), FRAME_SIZE);

    // See how many pages fit in given continuous block.
    page_count = (end - start) / FRAME_SIZE;
    size_t bitmap_size = BITMAP_GET_STORAGE_SIZE(page_count);

    // Since bitmap backing field is sotred in the beginning of available
    // address space.
    page_count -= round_up(bitmap_size, FRAME_SIZE) / FRAME_SIZE;
    bitmap_size = BITMAP_GET_STORAGE_SIZE(page_count);

    backing_field = (uint8_t*)start;
    page_start = end - page_count * FRAME_SIZE;

    // Assert that both bitmap backing_field and pages fit into given space.
    assert(start % FRAME_SIZE == 0 &&
           page_start % FRAME_SIZE == 0 && 
           end % FRAME_SIZE == 0);
    panic_if(start + bitmap_size > page_start || page_count == 0,
            "Frame init failed to create bitmap and corresponding pages in"
            " available in addresss range [%p, %p] with corresponding"
            " page count %u in starting from %p\n"
            "Bitmap size: %u\n",
            start, end, page_count, page_start, bitmap_size);

    bitmap_init(&bitmap, page_count, backing_field);

    interrupts_restore(enable);
}

/**
 * Allocate continuous sequence of physical frames.
 *
 * The allocated frames can be returned by frame_free. Note that multiple
 * frames allocated by this function can be returned separate by frame_free
 * and vice versa.
 *
 * @param count How many frames to allocate.
 * @param kseg0ptr Where to store virtual address (from kseg0) of the first frame
 *             in sequence.
 * @return Error code.
 * @retval EOK Frames allocated.
 * @retval ENOMEM Not enough memory.
 */
errno_t kframe_alloc(size_t count, uintptr_t* kseg0ptr) {
    bool enable = interrupts_disable();
    size_t idx;
    errno_t err = bitmap_find_range(&bitmap, count, false, &idx);
    switch (err) {
    case EOK:
        break;
    case ENOENT:
        interrupts_restore(enable);
        return ENOMEM;
    default:
        assert(false);
    }
    bitmap_fill_range(&bitmap, idx, count);
    *kseg0ptr = GET_ADDRESS(idx);

    interrupts_restore(enable);
    return EOK;
}

/**
 * Allocate continuous sequence of physical frames.
 *
 * The allocated frames can be returned by frame_free. Note that multiple
 * frames allocated by this function can be returned separate by frame_free
 * and vice versa.
 *
 * @param count How many frames to allocate.
 * @param phys Where to store physical address of the first frame in sequence.
 * @return Error code.
 * @retval EOK Frames allocated.
 * @retval ENOMEM Not enough memory.
 */
errno_t frame_alloc(size_t count, uintptr_t* phys) {
    errno_t err = kframe_alloc(count, phys);   
    if (err != EOK) {
        return err;
    }
    *phys = KSEG0_TO_PHYS(*phys);
    return EOK;
}
/**
 * Free continuous sequence of frames from kseg0 address.
 *
 * The returned frames were previously allocated by frame_alloc. Note that
 * multiple frames allocated by separate calls to frame_alloc can be freed
 * at once by this function and vice versa.
 *
 * @param count How many frames to free.
 * @param kseg0ptr Physical address of the first frame in sequence.
 * @return Error code.
 * @retval EOK Frames freed.
 * @retval ENOENT Invalid frame address or invalid count.
 * @retval EBUSY Some frames were not allocated (double free).
 */
errno_t kframe_free(size_t count, uintptr_t kseg0ptr) {
    bool enable = interrupts_disable();
    if (kseg0ptr % FRAME_SIZE != 0 ||
            !(kseg0ptr >= page_start && kseg0ptr <= end) ||
            !(kseg0ptr + count * FRAME_SIZE <= end)) {
        return ENOENT;
    }
    size_t idx = GET_INDEX(kseg0ptr);
    if (!bitmap_check_range_is(&bitmap, idx, count, true)) {
        interrupts_restore(enable);
        return EBUSY;
    }
    bitmap_clear_range(&bitmap, idx, count);

    interrupts_restore(enable);
    return EOK;
}

/**
 * Free continuous sequence of physical frames.
 *
 * The returned frames were previously allocated by frame_alloc. Note that
 * multiple frames allocated by separate calls to frame_alloc can be freed
 * at once by this function and vice versa.
 *
 * @param count How many frames to free.
 * @param phys Physical address of the first frame in sequence.
 * @return Error code.
 * @retval EOK Frames freed.
 * @retval ENOENT Invalid frame address or invalid count.
 * @retval EBUSY Some frames were not allocated (double free).
 */
errno_t frame_free(size_t count, uintptr_t phys) {
    return kframe_free(count, PHYS_TO_KSEG0(phys));
}

size_t get_page_count() {
    return page_count;
}

void debug_print_paging() {
    bool enable = interrupts_disable();
    printk("\nDEBUG PRINT PAGING\n");
    printk("Memory used for paging: %p <-> %p\n"
           "Page count: %u\n",
            page_start, end, page_count);

    size_t prev_idx = 0;
    size_t idx = 0;
    int current = bitmap_is_set(&bitmap, idx);
    int prev = bitmap_is_set(&bitmap, idx);
    for (idx = 0; idx < page_count; ++idx) {
        current = bitmap_is_set(&bitmap, idx);
        if (prev != current) {
            printk("\t%p:%u <-> %p:%u %s\n",
                    GET_ADDRESS(prev_idx), prev_idx,
                    page_start + ((idx - 1) * FRAME_SIZE), idx - 1,
                    (prev) ? "ALLOCATED" : "FREE");
            prev_idx = idx;
            prev = current;
        }
    }
    if (prev == current) {
        printk("\t%p:%u <-> %p:%u %s\n",
                GET_ADDRESS(prev_idx), prev_idx,
                GET_ADDRESS(idx - 1), idx - 1,
                (prev) ? "ALLOCATED" : "FREE");
    }
    printk("END DEBUG PRINT PAGING\n");
    interrupts_restore(enable);
}
