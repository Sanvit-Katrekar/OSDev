#include "kernel/kheap.h"
#include "libc/stdio.h"

uint8_t *kheap_start;
uint8_t *kheap_end;
uint8_t *kheap_curr;

static kheap_block_t *free_list;

void * kmalloc(size_t size) {
    size = align(size);
    kheap_block_t *cur_block = free_list;
    kheap_block_t *prev_block = NULL;

    while (cur_block) {
        // Find the first block that is large enough
        if (cur_block->size >= size) {
            if (cur_block->size > size + sizeof(kheap_block_t)) {
                // Split the block (Used + Free)
                kheap_block_t *new_block = (kheap_block_t *)((uint8_t *)cur_block + sizeof(kheap_block_t) + size);
                new_block->size = cur_block->size - size - sizeof(kheap_block_t);
                new_block->next = cur_block->next;

                cur_block->size = size;
                cur_block->next = NULL;

                if (prev_block) {
                    prev_block->next = new_block;
                } else {
                    free_list = new_block;
                }
            } else {
                // Remove the block from the free list
                if (prev_block) {
                    prev_block->next = cur_block->next;
                } else {
                    free_list = cur_block->next;
                }
            }

            return (void *)((uint8_t *)cur_block + sizeof(kheap_block_t));
        }

        prev_block = cur_block;
        cur_block = cur_block->next;
    }

    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) return;

    kheap_block_t *block = (kheap_block_t *)(ptr - sizeof(kheap_block_t));
    block->next = free_list; // Add the block to the free list
    free_list = block;

    // Coalesce free blocks
    kheap_block_t *cur_block = free_list;
    while (cur_block) {
        kheap_block_t *next_block = cur_block->next;
        // If the current block and the next block are contiguous
        if (next_block && (uint32_t)cur_block + cur_block->size + sizeof(kheap_block_t) == (uint32_t)next_block) {
            cur_block->size += next_block->size + sizeof(kheap_block_t);
            cur_block->next = next_block->next;
        } else {
            cur_block = next_block;
        }
    }
}

void print_kheap() {
    kheap_block_t *cur_block = free_list;
    while (cur_block) {
        printf("Free Blocks: %x, Size: %x\n", cur_block, cur_block->size);
        cur_block = cur_block->next;
    }
}

void kheap_init() {
    // Allocate 48MB of memory for the kernel heap
    kheap_start = (uint8_t *)KHEAP_START;
    kheap_end = kheap_start + KHEAP_INITIAL_SIZE;
    kheap_curr = kheap_start;

    free_list = (kheap_block_t *)kheap_start;
    free_list->size = KHEAP_INITIAL_SIZE - sizeof(kheap_block_t);
    free_list->next = NULL;
}