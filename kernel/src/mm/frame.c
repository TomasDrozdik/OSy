// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <mm/frame.h>
#include <types.h>

/**
 * Initializes frame allocator.
 *
 * Called once at system boot.
 */
void frame_init(void) {
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
    return ENOMEM;
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
 * @retval ENOENT Invalid frame address.
 * @retval EBUSY Some frames were not allocated (double free).
 */
errno_t frame_free(size_t count, uintptr_t phys) {
    return ENOIMPL;
}
