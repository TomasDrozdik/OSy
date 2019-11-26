// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <adt/list.h>
#include <debug/mm.h>
#include <mm/heap.h>
#include <lib/print.h>

#include <lib/print.h>

/** Minimal size of an allocated payload.
 *  Anything below this size is increased to it.
 */
#define MIN_ALLOCATION_SIZE 4

/** Gets pointer to the header of a block whith the corresponding payload.
 *
 * @param PTR Pointer to payload, i.e. returned by kmalloc.
 * @returns Pointer to block_header_t of given block.
 */
#define HEADER_FROM_PAYLOAD(PTR) ((block_header_t*)((uintptr_t)PTR - sizeof(block_header_t)))

/** Gets pointer to the payload of a block from givne payload link.
 *
 * @param PTR Pointer to payload, i.e. returned by kmalloc.
 * @returns Pointer to block_header_t of given block.
 */
#define PAYLOAD_FROM_HEADER(HEADERPTR) ((void*)((uintptr_t)HEADERPTR + sizeof(block_header_t)))

/** Get the pointer to the block_header structure from given link pointer.
 *
 * @param LINKPTR Pointer to link_t.
 * @returns Pointer to block_header.
*/
#define HEADER_FROM_LINK(LINKPTR) list_item( \
        LINKPTR, block_header_t, link)

/** List of all blocks in heap. */
static list_t blocks;

/** Each block has a header which contains its size i.e size of block header +
 *  block payload, free flag and a link which links it to all othe blocks.
 */
typedef struct block_header {
    size_t size;
    bool free;
    link_t link;
} block_header_t;

/** Align the pointer.
 * @param ptr Pointer to align.
 * @param size Alignt according to the given size.
 * @returns Aligned pointer.
 */
static inline uintptr_t align(uintptr_t ptr, size_t size);

//static void debug_print_block_list(list_t* blocks) {
//    dprintk("%pL\n", blocks);
//
//    list_foreach(*blocks, block_header_t, link, header) {
//        dprintk(
//                "\th[p: %p, size: %u, free: %u, link->prev: %p, link->next: %p] ->\n",
//                &header->link, header->size, header->free, header->link.prev,
//                header->link.next);
//    }
//}

/** Initialized heap.
 *
 * Uses pointer to kernel end and size of the available memory to determine
 * pointer to the end of available memory i.e. heap. And sets up the initial
 * free header spanning the whole memory.
 */
void heap_init(void) {
    list_init(&blocks);

//    dprintk("KERNEL_END = %p\n", (void*)debug_get_kernel_endptr());
//    dprintk("INIT: &blocks = %p\n", &blocks);

    uintptr_t start_ptr = align(debug_get_kernel_endptr(), MIN_ALLOCATION_SIZE);
    block_header_t* initial_header = (block_header_t*)start_ptr;
    initial_header->size = debug_get_base_memory_size();
    initial_header->free = true;

    list_append(&blocks, &initial_header->link);

//    debug_print_block_list(&blocks);
//    dprintk("\n\n");
}

void* kmalloc(size_t size) {
    size = align(size, MIN_ALLOCATION_SIZE);
    size_t actual_size = size + sizeof(block_header_t);

//    dprintk("Searching for block of size %u, actual size %u\n", size, actual_size);

    list_foreach(blocks, block_header_t, link, header) {
        if (header->free) {
            if (header->size == actual_size) {

//                dprintk("Header[%p] exact size found\n", &header->link);

                header->free = false;

//                debug_print_block_list(&blocks);
//                dprintk("Returning pointer %p\n\n", PAYLOAD_FROM_HEADER(header));

                return PAYLOAD_FROM_HEADER(header);
            } else if (header->size >= actual_size + sizeof(block_header_t)
                            + MIN_ALLOCATION_SIZE) {

//                dprintk("Header[%p] sufficient size found\n", &header->link);

                // Create new header inside of this memory block, with new
                // header as a free one and this one would serve as allocated
                // block for requested size.
                block_header_t* new_header = (block_header_t*)((uintptr_t)header + actual_size);
                new_header->size = header->size - actual_size;
                header->size = actual_size;

                new_header->free = true;
                header->free = false;

                list_add(&header->link, &new_header->link);

//                debug_print_block_list(&blocks);
//                dprintk("Returning pointer %p\n\n", PAYLOAD_FROM_HEADER(header));

                return PAYLOAD_FROM_HEADER(header);
            } else {
//                dprintk("Requested size %u is less than minimal size %u\n",
//                        actual_size,
//                        actual_size + sizeof(block_header_t) + MIN_ALLOCATION_SIZE);
            }
        }
    }
//    dprintk("Requested size not found\n");
//    debug_print_block_list(&blocks);
//    dprintk("\n\n");
    return NULL;
}

void kfree(void* ptr) {
    block_header_t* header = HEADER_FROM_PAYLOAD(ptr);
    assert(header->free == false);

//    dprintk("Freeing pointer %p cooresponding to link %p\n",
//            ptr, &header->link);

    header->free = true;

    // Compacting with next
    link_t* next_link = header->link.next;
    if (valid_link(blocks, next_link)) {
        block_header_t* next_header = HEADER_FROM_LINK(next_link);
        if (next_header->free) {
            header->size += next_header->size;
            list_remove(next_link);
        }
    }

    // Compacting with prev
    link_t* prev_link = header->link.prev;
    if (valid_link(blocks, prev_link)) {
        block_header_t* prev_header = HEADER_FROM_LINK(header->link.prev);
        if (prev_header->free) {
            prev_header->size += header->size;
            list_remove(&header->link);
        }
    }

//    debug_print_block_list(&blocks);
//    dprintk("\n\n");
}

static inline uintptr_t align(uintptr_t ptr, size_t size) {
    size_t remainder;
    remainder = ptr % size;
    if (remainder == 0) {
        return ptr;
    }
    return ptr - remainder + size;
}
