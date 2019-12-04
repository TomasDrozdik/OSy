// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/list.h>
#include <debug/mm.h>
#include <mm/heap.h>

#include <lib/print.h>

/** Minimal size of an allocated payload.
 *  Anything below this size is increased to it.
 */
#define MIN_ALLOCATION_SIZE 4

/** Checks if given link valid i.e. not head of the list.
 *
 *  Explanation: Head of the list does not contain block_header structure.
 *
 * @param LINKPTR Pointer to link structure.
 * @returns True if valid, false otherwise.
 */
#define VALID_HEADER_LINK(LINKPTR) (LINKPTR != &blocks.head)

/** Gets pointer to the header of a block whith the corresponding payload.
 *
 * @param PTR Pointer to payload, i.e. returned by kmalloc.
 * @returns Pointer to block_header_t of given block.
 */
#define HEADER_FROM_PAYLOAD(PTR) \
    ((block_header_t*)((uintptr_t)PTR - sizeof(block_header_t)))

/** Gets pointer to the payload of a block from givne payload link.
 *
 * @param PTR Pointer to payload, i.e. returned by kmalloc.
 * @returns Pointer to block_header_t of given block.
 */
#define PAYLOAD_FROM_HEADER(HEADERPTR) \
    ((void*)((uintptr_t)HEADERPTR + sizeof(block_header_t)))

/** Get the pointer to the block_header structure from given link pointer.
 *
 * @param LINKPTR Pointer to link_t.
 * @returns Pointer to block_header.
 */
#define HEADER_FROM_LINK(LINKPTR) \
    list_item(LINKPTR, block_header_t, link)

/** Computes size of given block.
 * By comparing its link with link of next block. If next block is not valid
 * i.e. its a head of the list (i.e. last item in memory) then just compare it
 * to the memory endptr.
 * @param HEADERPTR Pointer to the header of a block to check.
 * @returns Size of a block in size_t type.
 */
#define BLOCK_SIZE(HEADERPTR) ((size_t) \
    (VALID_HEADER_LINK(HEADERPTR->link.next) ? \
        (uintptr_t)HEADERPTR->link.next - (uintptr_t)&HEADERPTR->link : \
        end_ptr - (uintptr_t)HEADERPTR))

/** Checks if given header is free.
 * By checking validity of free_link. This means that each free block HAS to
 * have invalidated free_link by calling @link_init on it.
 * @param HEADERPTR Pointer to the header of block to check.
 * @returns True if given block is in free_list, false otherwise.
 */
#define IS_FREE(HEADERPTR) \
    link_is_connected(&HEADERPTR->free_link)

/** Indicates if heap_init has been called. */
static bool heap_initialized = false;

/** List of all blocks in heap. */
static list_t blocks;
static list_t free_blocks;
static uintptr_t end_ptr;

/** Each block has a header which contains its size, free flag and link which
 *  links it to all othe blocks.
 */
typedef struct block_header {
    link_t link;
    link_t free_link;
} block_header_t;

/** Initialized heap.
 *
 * Uses pointer to kernel end and size of the available memory to determine
 * pointer to the end of available memory i.e. heap.
 */
static void heap_init(void);

/** Align the pointer.
 * @param ptr Pointer to align.
 * @param size Alignt according to the given size.
 * @returns Aligned pointer.
 */
static inline uintptr_t align(uintptr_t ptr, size_t size);

/** Compact two neighboring links.
 * Do check whether both are free and then compacts both to prev header.
 * @param prev Link before @next.
 * @param next Link after @prev.
 */
static inline void compact(link_t* prev, link_t* next);

void* kmalloc(size_t size) {
    if (!heap_initialized) {
        heap_init();
        heap_initialized = true;
    }

    size = align(size, MIN_ALLOCATION_SIZE);
    size_t actual_size = size + sizeof(block_header_t);

    list_foreach(free_blocks, block_header_t, free_link, header) {
        if (BLOCK_SIZE(header) == actual_size) {
            list_remove(&header->free_link);
            return PAYLOAD_FROM_HEADER(header);
        } else if (BLOCK_SIZE(header) >= actual_size + sizeof(block_header_t)
                        + MIN_ALLOCATION_SIZE) {

            // Create new header inside of this memory block, with new
            // header as a free one and this one would serve as allocated
            // block for requested size.
            block_header_t* new_header = (block_header_t*)((uintptr_t)header + actual_size);
            list_add(&header->link, &new_header->link);
            list_append(&free_blocks, &new_header->free_link);

            list_remove(&header->free_link);
            link_init(&header->free_link);

            return PAYLOAD_FROM_HEADER(header);
        }
    }
    return NULL;
}

void kfree(void* ptr) {
    block_header_t* header = HEADER_FROM_PAYLOAD(ptr);
    assert(!link_is_connected(&header->free_link));

    list_prepend(&free_blocks, &header->free_link);

    compact(header->link.prev, &header->link);
    compact(&header->link, header->link.next);
}

static void heap_init(void) {
    list_init(&blocks);
    list_init(&free_blocks);

    uintptr_t start_ptr = align(debug_get_kernel_endptr(), MIN_ALLOCATION_SIZE);
    end_ptr = debug_get_base_memory_endptr();

    block_header_t* initial_header = (block_header_t*)start_ptr;

    list_append(&blocks, &initial_header->link);
    list_append(&free_blocks, &initial_header->free_link);
}

static inline uintptr_t align(uintptr_t ptr, size_t size) {
    size_t remainder;
    remainder = ptr % size;
    if (remainder == 0) {
        return ptr;
    }
    return ptr - remainder + size;
}

static inline void compact(link_t* prev, link_t* next) {
    block_header_t* prev_header = VALID_HEADER_LINK(prev) ?
            HEADER_FROM_LINK(prev) : NULL;
    block_header_t* next_header = VALID_HEADER_LINK(next) ?
            HEADER_FROM_LINK(next) : NULL;
    if (prev_header && next_header &&
            IS_FREE(prev_header) && IS_FREE(next_header)) {
        list_remove(&next_header->link);
        list_remove(&next_header->free_link);
    }
}
