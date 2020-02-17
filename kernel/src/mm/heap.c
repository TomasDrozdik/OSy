// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/list.h>
#include <debug/mm.h>
#include <exc.h>
#include <lib/print.h>
#include <mm/frame.h>
#include <mm/heap.h>
#include <proc/sem.h>
#include <proc/thread.h>
#include <utils.h>

/** Minimal size of an allocated payload.
 *  Anything below this size is increased to it.
 */
#define MIN_ALLOCATION_SIZE 4

/** Gets pointer to the header of a block whith the corresponding payload.
 *
 * @param ptr Pointer to payload, i.e. returned by kmalloc.
 * @returns Pointer to block_header_t of given block.
 */
#define HEADER_FROM_PAYLOAD(ptr) \
    ((block_header_t*)((uintptr_t)(ptr) - sizeof(block_header_t)))

/** Gets pointer to the payload of a block from givne payload link.
 *
 * @param headerptr Pointer to payload, i.e. returned by kmalloc.
 * @returns Pointer to block_header_t of given block.
 */
#define PAYLOAD_FROM_HEADER(headerptr) \
    ((void*)((uintptr_t)(headerptr) + sizeof(block_header_t)))

/** Get the pointer to the block_header structure from given link pointer.
 *
 * @param linkptr Pointer to link_t.
 * @returns Pointer to block_header.
 */
#define HEADER_FROM_LINK(linkptr) \
    list_item((linkptr), block_header_t, link)

/** Computes size of given block.
 * By comparing its link with link of next block. If next block is not valid
 * i.e. its a head of the list (i.e. last item in memory) then just compare it
 * to the memory endptr.
 * @param headerptr Pointer to the header of a block to check.
 * @returns Size of a block in size_t type.
 */
#define BLOCK_SIZE(headerptr) \
    ((size_t)(valid_link(blocks, (headerptr)->link.next) ? (uintptr_t)(headerptr)->link.next - (uintptr_t) & (headerptr)->link : end - (uintptr_t)(headerptr)))

/** Checks if given header is free.
 * By checking validity of free_link. This means that each free block HAS to
 * have invalidated free_link by calling @link_init on it.
 * @param HEADERPTR Pointer to the header of block to check.
 * @returns True if given block is in free_list, false otherwise.
 */
#define IS_FREE(headerptr) \
    link_is_connected(&(headerptr)->free_link)

/** List of all blocks in heap. */
static list_t blocks;
static list_t free_blocks;
static uintptr_t end;

/** Each block has a header which contains its size i.e size of block header +
 *  block payload, free flag and a link which links it to all othe blocks.
 */
typedef struct block_header {
    link_t link;
    link_t free_link;
} block_header_t;

/** Compact two neighboring links.
 * Do check whether both are free and then compacts both to prev header.
 * @param prev Link before @next.
 * @param next Link after @prev.
 */
static inline void compact(link_t* prev, link_t* next);

/** Attempt to make some calculation of how big the heap should be.
 *  
 *  Take into consideration that thread stack is allocated from frame allocator.
 */
static size_t count_needed_pages() {
    const int32_t total_page_count = get_page_count();
    const int32_t size = total_page_count * PAGE_SIZE;
    const int32_t fitting_threads = size / (sizeof(thread_t) + THREAD_STACK_SIZE);
    size_t page_count = (fitting_threads * (sizeof(thread_t) + sizeof(sem_t))) / PAGE_SIZE;
    if (page_count == 0) {
        page_count = 1;
    }
    panic_if(total_page_count - page_count < page_count,
            "Heap has more pages than paging.\n");
    panic_if(total_page_count - page_count <= 0 || page_count <= 0,
            "Not enough memory to run both heap and frame allocator\n"
            "\tPage count: %u\n"
            "\tHeap page count: %u\n"
            "\tFrame allocator page count: %u\n",
            total_page_count, page_count, total_page_count - page_count);
    dprintk("Page division\n"
            "\tPage count: %u\n"
            "\tHeap page count: %u\n"
            "\tFrame allocator page count: %u\n",
            total_page_count, page_count, total_page_count - page_count);
    return page_count;
}

void heap_init(void) {
    bool enable = interrupts_disable();

    list_init(&blocks);
    list_init(&free_blocks);

    // Prealloc precalculated ammount of pages.
    // Since our allocator requires continuous memory block we are not
    // allocating pages dynamically but rather statically preallocate them.
    uintptr_t start;
    size_t page_count = count_needed_pages();
    kframe_alloc(page_count, &start);
    end = start + page_count * FRAME_SIZE;

    block_header_t* initial_header = (block_header_t*)start;

    list_append(&blocks, &initial_header->link);
    list_append(&free_blocks, &initial_header->free_link);

    interrupts_restore(enable);
}

void* kmalloc(size_t size) {
    bool enable = interrupts_disable();

    size = round_up(size, MIN_ALLOCATION_SIZE);
    size_t actual_size = size + sizeof(block_header_t);

    list_foreach(free_blocks, block_header_t, free_link, header) {
        if (BLOCK_SIZE(header) == actual_size) {
            list_remove(&header->free_link);
            link_init(&header->free_link);

            interrupts_restore(enable);
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

            interrupts_restore(enable);
            return PAYLOAD_FROM_HEADER(header);
        }
    }
    interrupts_restore(enable);
    return NULL;
}

void kfree(void* ptr) {
    bool enable = interrupts_disable();

    block_header_t* header = HEADER_FROM_PAYLOAD(ptr);
    panic_if(IS_FREE(header),
            "Freeing memory block which is not allocated.\n");

    list_prepend(&free_blocks, &header->free_link);

    compact(header->link.prev, &header->link);
    compact(&header->link, header->link.next);

    interrupts_restore(enable);
}

void debug_print_heap() {
    printk("\nDEBUG PRINT HEAP\n");
    printk("\tBLOCK_LIST: %pL\n", &blocks);

    list_foreach(blocks, block_header_t, link, header) {
        printk(
                "\th[p: %p, size: %u, free: %u] ->\n",
                &header->link, BLOCK_SIZE(header), IS_FREE(header));
    }
    printk("END DEBUG PRINT HEAP\n");
}

static inline void compact(link_t* prev, link_t* next) {
    block_header_t* prev_header = valid_link(blocks, prev) ? HEADER_FROM_LINK(prev) : NULL;
    block_header_t* next_header = valid_link(blocks, next) ? HEADER_FROM_LINK(next) : NULL;
    if (prev_header && next_header && IS_FREE(prev_header) && IS_FREE(next_header)) {
        list_remove(&next_header->link);
        list_remove(&next_header->free_link);
    }
}
