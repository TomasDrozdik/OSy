// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <mm/frame.h>
#include <types.h>


// Max memory size is set to 512MB
#define MAX_MEMORY_SIZE (512 * 1024 * 1024)

// Max number of nodes in a tree according to given page size 
#define MAX_TREE_SIZE \
    (MAX_MEMORY_SIZE / FRAME_SIZE * 2)

#define LEFT(IDX) (2 * (IDX + 1) - 1)
#define RIGHT(IDX) (2 * (IDX + 1))

#define SIZE_FROM_DEPTH \
    ()

typedef enum {
    FREE,
    FULL,
    SPLIT,
} node_t;

size_t memory_size;
uintptr_t start_ptr;
node_t buddy_tree[MAX_TREE_SIZE];

static inline uintptr_t align(uintptr_t ptr, size_t size) {
    size_t remainder;
    remainder = ptr % size;
    if (remainder == 0) {
        return ptr;
    }
    return ptr - remainder + size;
}

static inline uint32_t get_next_power_of_2(uint32_t n) {
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

static inline uint32_t is_power_of_2(uint32_t n) {
    return n && !(n & (n - 1));
}

static size_t alloc(size_t idx, size_t current_node_size, size_t size) {
    if (buddy_tree[idx] == FULL) {
        return -1;
    }

    if (buddy_tree[idx] == FREE) {
        if (current_node_size < 2 * size) {
            return idx;
        } else {
            buddy_tree[idx] = SPLIT;
            buddy_tree[LEFT(idx)] = FREE;
            buddy_tree[RIGHT(idx)] = FREE;
        }
    }

    if (buddy_tree[idx] == SPLIT) {
        size_t ret;
        
        ret = alloc(LEFT(idx), current_node_size / FRAME_SIZE, size);
        if (ret != -1) {
            return ret;
        }

        ret = alloc(RIGHT(idx), current_node_size / FRAME_SIZE, size);
        if (ret != -1) {
            return ret;
        }
    }

    assert(false);
    return -1;
}

/**
 * Initializes frame allocator.
 *j
 * Called once at system boot.
 */
void frame_init(void) {
    memory_size = align(debug_get_memory_size(), FRAME_SIZE);
    start_ptr = align(debug_get_kernel_endptr(), FRAME_SIZE);

    buddy_tree[0] = FREE;
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
    size_t size = count * FRAME_SIZE;
    size_t idx;

    idx = alloc(0, memory_size, size);

    if (idx == -1) {
        return ENOMEM;
    }

    idx += 1;
    size_t next_power_of_2 = get_next_power_of_2(idx);
    size_t items_in_given_depth = is_power_of_2(idx) ? idx : next_power_of_2;
    size_t initial_idx_in_given_depth = is_power_of_2(idx) ? idx :
            items_in_given_depth >> 1;

    *phys = start_ptr + (memory_size / items_in_given_depth *
            (idx - initial_idx_in_given_depth));
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
 * @retval ENOENT Invalid frame address.
 * @retval EBUSY Some frames were not allocated (double free).
 */
errno_t frame_free(size_t count, uintptr_t phys) {
    
    return ENOIMPL;
}
