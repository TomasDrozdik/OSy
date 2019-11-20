// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug/mm.h>
#include <main.h>
#include <mm/heap.h>
#include <lib/print.h>

//Structure representing a block of memory.
typedef struct block {
    size_t size;
    struct block* next;
    struct block* previous;
} block_t;

#define BLOCK_SIZE 4

#define BLOCK_MEM(ptr) ((void*)((unsigned long)ptr + sizeof(block_t)))
#define BLOCK_HEADER(ptr) ((void*)((unsigned long)ptr - sizeof(block_t)))

//Pointer to the beginning of the list, holding all blocks of free space
//Inspired by https://github.com/andrestc/linux-prog/blob/master/ch7/malloc.c
static block_t* free_list_head = NULL;

static uintptr_t last_alloc;

/** Indicates if heap_init has been called. */
static bool heap_initialized = false;

/** Poiter to the start of the available memory. */
static uintptr_t start_ptr;

/** Poiter to the end of the available memory. */
static uintptr_t end_ptr;

block_t* split(block_t* block, size_t size);

void free_list_remove(block_t* block);

void free_list_add(block_t* block);

void defragment_list(void);

void stats(char* prefix);

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

//Removes a block from the list
void free_list_remove(block_t* block) {
    if (!block->previous) {
        if (block->next) {
            free_list_head = block->next;
		} else {
            free_list_head = NULL;
		}
    } else {
        block->next->previous = block->previous;
        if(block->next){block->previous->next = block->next;}
	}
}

//Adds a block to the list, list stays sorted by the blocks' begin address.
void free_list_add(block_t* block) {
    block->next = NULL;
    block->previous = NULL;

    if (!free_list_head || (unsigned long)free_list_head > (unsigned long)block) {
        if (free_list_head) {
            free_list_head->previous = block;
		}
        block->next = free_list_head;
        free_list_head = block;
    } else {
        block_t* current = free_list_head;
        while (current->next&&(unsigned long)current->next<(unsigned long)block) {
            current = current->next;
        }
        block->next = current->next;
        current->next = block;
        block->previous = current;
        block->next->previous = block;
	}
}

static void heap_init(void) {
    start_ptr = align((uintptr_t)&_kernel_end, BLOCK_SIZE);
    end_ptr = debug_get_base_memory_endptr();
    last_alloc = start_ptr;
}

void heap_init(void) {
}

void* kmalloc(size_t size) {
    
    if (!heap_initialized) {
        heap_init();
        heap_initialized = true;
    }
    
    // Check if the size requested is actually valid.
    if (size>end_ptr-start_ptr) {
        return NULL;
    }

    block_t* best = NULL;
	block_t* current = free_list_head;
    while (current->next) {
		if (current->size >= size) {
			//found a perfectly sized block
            if (current->size == size) {
				return BLOCK_MEM(current);
			}
            if (!best||best->size>current->size) {
                best = current;
			}
		}
		current = current->next;
    }
    if (best) {
		free_list_remove(best);
	}

	//if the smallest block is still bigger than required, split it.
    if (best && best->size > size) {
        block_t* newblock = split(best, size);
        free_list_add(newblock);
	}

	//Free_list doesnt have a sufficient block.Raise end memory pointer;
    if (!best) {
		if (last_alloc + size <= end_ptr) {
            uintptr_t ptr = last_alloc;
			last_alloc = align(last_alloc + size, BLOCK_SIZE);
            return (void*)ptr;
		}
		return NULL;
	}
	return BLOCK_MEM(best);
}

void kfree(void* ptr) {
    free_list_add(BLOCK_HEADER(ptr));
    stats("Before defrag.");
    defragment_list();
}

//Defragments the list,a.k.a. if two blocks are continuosly addressed, it will merge them.
void defragment_list(void) {

	block_t* current = free_list_head;

    while (current->next) {
		if (current + current->size + sizeof(block_t) == current->next) {
            current->size += current->next->size + sizeof(block_t);
			current->next = current->next->next;
            if (current->next) {
				current->next->previous = current;
                continue;
			} else {
                break;
			}
		}
        current = current->next;
    }
    stats("After merge");

	if (align((uintptr_t)current + current->size+sizeof(block_t), BLOCK_SIZE) == last_alloc) {
        last_alloc = align(last_alloc - current->size, BLOCK_SIZE);
		free_list_remove(current);
	}
}

//splits the memory block, so malloc only allocates required memory, rest is added to the free list.
block_t* split(block_t* block, size_t size) {

    void* memory_block = BLOCK_MEM(block);
    block_t* ptr = (block_t*)((unsigned long)memory_block + size);
    ptr->size = block->size - (size + sizeof(block_t));
    block->size = size;
    return ptr;
}

//Debug info
void stats(char* prefix) {
    printk("[%s] program break: %p\n", prefix, last_alloc);
    block_t* ptr = free_list_head;
    printk("[%s] free list: \n", prefix);
    int c = 0;
    while (ptr) {
        printk("(%d) <%p> (size: %d)\n", c, ptr, ptr->size);
        ptr = ptr->next;
        c++;
    }
}

static inline uintptr_t align(uintptr_t ptr, size_t size) {
    size_t remainder;
    if ((remainder = ptr % size) == 0) {
        return ptr;
    }
    return ptr - remainder + size;
}


