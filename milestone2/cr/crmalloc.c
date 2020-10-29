#include <stdio.h>
#include "crmalloc.h"
#include "nvstore.h"

/* Instance getter for memory manager */
static struct memory_manager *mm_instance() {
    static struct memory_manager mm;
    static bool constructed = false;
    if (!constructed) {
        list_init(&mm.free);
    }
    return &mm;
}

/* Given tag, determine size of payload it guards */
static inline size_t tag_payload_size(union boundary_tag *tag) {
    return tag->data.payload_size;
}

/* Determine if tag refers to a block that is in use */
static inline bool tag_inuse(union boundary_tag *tag) {
    return tag->data.inuse;
}

/* Determine payload size of block */
static inline size_t block_payload_size(struct block *b) {
    return tag_payload_size(&b->tag);
}

/* Determine if block is in use */
static inline bool block_inuse(struct block *b) {
    return tag_inuse(&b->tag);
}

/* From a pointer to a payload, get the block */
static inline struct block *payload_to_block(void *payload) {
    return (struct block*) (
        ((void *) payload) - offsetof(struct block, payload)
    );
}

/* Getter for left tag of block */
static inline union boundary_tag *block_left_tag(struct block *b) {
    return &b->tag;
}

/* Getter for right tag in block */
static inline union boundary_tag *block_right_tag(struct block *b) {
    return (union boundary_tag *) (
        ((void *) b) + sizeof(struct block) + block_payload_size(b)
    );
}

/* Given a block, find the block on its left */
static inline struct block *block_on_left(struct block *block) {

    // Get pointer to right boundary tag for block on left
    void *b = (void *) block;
    b -= sizeof(union boundary_tag);

    // Guard if current block is leftmost in the set of pages
    if (b <= block->mem_lo) { return NULL; }

    // Determine payload size and back up the pointer
    size_t payload_size = tag_payload_size((union boundary_tag *) b);
    void *payload = b - payload_size;

    return payload_to_block(payload);

}

/* Given a block, find the block on its right */
static inline struct block *block_on_right(struct block *block) {
    void *b = (void *) block;
    b += block_payload_size(block) 
         + sizeof(struct block) 
         + sizeof(union boundary_tag);
    return b > ((void *) block->mem_hi) ? NULL : (struct block *) b;
}

/**
 * Attempt to split block. Assumes given block is large enough to store
 * at least the requested_size
 */
static struct block *split_block(struct block *b, size_t requested_size) {

    // Only split if the remainder is large enough to hold a payload
    int64_t remainder_size = (int64_t) block_payload_size(b)
                             - (int64_t) METADATA_SIZE
                             - requested_size;
    if (remainder_size > 0) {

        // Shrink payload size of current block
        block_left_tag(b)->data.payload_size = requested_size;
        block_right_tag(b)->data.payload_size = requested_size;

        // Set boundary tags of next block
        struct block *next_b = ((void *) block_right_tag(b))
                               + sizeof(union boundary_tag);
        block_left_tag(next_b)->data.payload_size = (size_t) remainder_size;
        block_left_tag(next_b)->data.inuse = false;
        block_right_tag(next_b)->data.payload_size = (size_t) remainder_size;
        block_right_tag(next_b)->data.inuse = false;

        // Set absolute memory boundaries of next block
        next_b->mem_lo = b->mem_lo;
        next_b->mem_hi = b->mem_hi;

        // Add next block to free list
        struct memory_manager *mm = mm_instance();
        list_push_back(&mm->free, &next_b->elem);

    }

    // Mark current block as inuse
    block_left_tag(b)->data.inuse = true;
    block_right_tag(b)->data.inuse = true;

    return b;
}

/* Allocate pages of memory to satisfy request size */
static struct block *make_block(size_t payload_size) {

    // Determine number of pages to request
    size_t block_size = payload_size + METADATA_SIZE;
    size_t npages = block_size/PAGESIZE + (block_size % PAGESIZE != 0);

    // Request memory
    struct block *b = nvstore_allocpage(npages);
    if (!b) {
        printf("ERROR: make_block() failed on size <%ld>\n", payload_size);
        return NULL;
    }

    // Initialize boundary tags
    block_left_tag(b)->data.payload_size = npages*PAGESIZE-METADATA_SIZE;
    block_left_tag(b)->data.inuse = true;
    block_right_tag(b)->data.payload_size = npages*PAGESIZE-METADATA_SIZE;
    block_right_tag(b)->data.inuse = true;

    // Initialize absolute memory boundaries
    b->mem_lo = (void *) b;
    b->mem_hi = ((void *) b) + npages*PAGESIZE;

    // Split block to free any unused spaces
    return split_block(b, block_size);

}

/* Search for free block with large enough payload */
static struct block *find_reusable_block(size_t size) {
    struct memory_manager *mm = mm_instance();
    struct list_elem *e = list_begin(&mm->free);
    while (e != list_end(&mm->free)) {
        struct block *b = list_entry(e, struct block, elem);
        if (block_payload_size(b) >= size) {
            list_remove(e);
            return split_block(b, size);
        }
        e = list_next(e);
    }
    return NULL;
}

/* Attempt to coalesce block with its neighbors */
static struct block *coalesce(struct block *b) {

    // Mark block as free
    block_left_tag(b)->data.inuse = false;
    block_right_tag(b)->data.inuse = false;

    // Get neighboring blocks
    struct block *left_b = block_on_left(b);
    struct block *right_b = block_on_right(b);

    // Coalesce right
    if (right_b && !block_inuse(right_b)) {

        // Remove right block from free list and combine sizes
        list_remove(&right_b->elem);
        size_t new_size = block_payload_size(b) 
                          + block_payload_size(right_b) 
                          + METADATA_SIZE;
        
        // Update right tag
        block_right_tag(right_b)->data.payload_size = new_size;
        block_right_tag(right_b)->data.inuse = false;

        // Update left tag
        block_left_tag(b)->data.payload_size = new_size;

    }

    // Coalesce left
    if (left_b && !block_inuse(left_b)) {

        // Remove left block from free list and combine sizes
        list_remove(&left_b->elem);
        size_t new_size = block_payload_size(b)
                          + block_payload_size(left_b)
                          + METADATA_SIZE;

        // Update left tag
        block_left_tag(left_b)->data.payload_size = new_size;
        block_left_tag(left_b)->data.inuse = false;

        // Update right tag
        block_right_tag(b)->data.payload_size = new_size;
        block_right_tag(b)->data.inuse = false;

        // Start of block is now where left block started
        b = left_b;

    }

    return b;
}

/* ---------- USER INTERFACE ---------- */

/* Return a usable block with given minimum size */
void *crmalloc(size_t size) {

    // Ignore spurious requests
    if (size == 0) { return NULL; }

    // Search reusable free blocks
    struct block *b = find_reusable_block(size);
    if (b) { return b->payload; }

    // No suitable free blocks available; allocate new block
    b = make_block(size);
    return b->payload;

}

/* Free a block for reuse, coalesce when possible */
void crfree(void *ptr) {

    // Ignore spurious requests
    if (ptr == NULL) { return; }

    // Instance getter
    struct memory_manager *mm = mm_instance();

    // Get block from payload
    struct block *b = payload_to_block(ptr);

    // Coalesce with neighbors
    b = coalesce(b);

    // Add to free list
    list_push_back(&mm->free, &b->elem);

}

void *crrealloc(void *ptr, size_t size) {

    // Reduce spurious requests
    if (ptr == NULL) { return crmalloc(size); }
    if (size == 0) {
        crfree(ptr);
        return NULL;
    }

    // Keep track of original block
    char *old_payload = (char *) ptr;
    struct block *old_b = payload_to_block(old_payload);

    // If new block is smaller than old block, split
    if (size <= block_payload_size(old_b)) {
        struct block *b = split_block(old_b, size);
        return b->payload;
    }

    // If block on right is free and large enough, coalesce
    size_t combined_size = METADATA_SIZE
                           + block_payload_size(old_b)
                           + block_payload_size(block_on_left(old_b));
    if (combined_size >= size) {
        struct block *right_b = block_on_right(old_b);
        if (!block_inuse(right_b)) {
            
            // Remove right block from free list
            list_remove(&right_b->elem);

            // Update right boundary tag
            block_right_tag(right_b)->data.payload_size = combined_size;
            block_right_tag(right_b)->data.inuse = true;

            // Update left boundary tag
            block_left_tag(old_b)->data.payload_size = combined_size;
            block_left_tag(old_b)->data.inuse = true;

            return old_b->payload;
        }
    }

    // Otherwise, malloc a new block, copy memory, free old block
    char *new_payload = crmalloc(size);
    for (size_t i = 0; i < block_payload_size(old_b); i++) {
        new_payload[i] = old_payload[i];
    }
    crfree(old_payload);
    return new_payload;
}