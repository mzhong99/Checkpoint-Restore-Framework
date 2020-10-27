// Simple test that covers each of the following cases
//
//  ( 1) Malloc requires a request for more pages, but request is
//         sized so that it cannot split any extra memory into a
//         separate free block.
//  ( 2) Malloc requires a request for more pages, and the unused
//         portion of memory is large enough to split into a
//         separate free block.
//  ( 3) Malloc finds a suitable block for reuse, but cannot split
//         any extra memory into a separate free block.
//  ( 4) Malloc finds a suitable block for reuse, and the unused
//         portion of memory is large enough to split into a
//         separate free block.
//
//  ( 5) Free cannot coalesce
//  ( 6) Free should coalesce to the left
//  ( 7) Free should coalesce to the right
//
//  ( 8) Realloc attempts to make block smaller
//  ( 9) Realloc needs to make block larger, and the block on the
//         right is large enough to accomodate the additional size.
//  (10) Realloc needs to make block larger, but extending the
//         length of the current block is not an option. This
//         option results in a call to both malloc and free.
//
// The diagram below visualizes the expected results of each stage
//   of the test, where "I" and "F" indicate a tag marking a block
//   as "inuse" or free, respectively.
//
//  +---+-----------------------------------------------+---+
//  | I | 1...                                     ...1 | I | ( 1)
//  +---+-----------------------------------------------+---+
//                              |
//                              V
//  +---+-----------+---+---+---------------------------+---+
//  | I | 1... ...1 | I | F |                           | F | ( 8)
//  +---+-----------+---+---+---------------------------+---+
//                              |
//                              V
//  +---+-----------+---+---+-------+---+---+-----------+---+
//  | I | 1... ...1 | I | I | 2.... | I | F |           | F | ( 4)
//  +---+-----------+---+---+-------+---+---+-----------+---+
//                              |
//                              V
//  +---+-----------+---+---+-------+---+---+-----------+---+
//  | I | 1... ...1 | I | I | 2.... | I | I | 3... ...3 | I | ( 3)
//  +---+-----------+---+---+-------+---+---+-----------+---+
//                              |
//                              V
//  +---+-----------+---+---+-------+---+---+-----------+---+
//  | I | 1... ...1 | I | F |       | F | I | 3... ...3 | I | (10)->(2,5)
//  +---+-----------+---+---+-------+---+---+-----------+---+
//  +---+----------------------+---+---+----------------+---+
//  | I | 2...            ...2 | I | F |                | F |
//  +---+----------------------+---+---+----------------+---+
//                              |
//                              V
//  +---+-----------+---+---+-------+---+---+-----------+---+
//  | I | 1... ...1 | I | F |       | F | I | 3... ...3 | I | ( 9)
//  +---+-----------+---+---+-------+---+---+-----------+---+
//  +---+-------------------------------+---+---+-------+---+
//  | I | 2...                     ...2 | I | F |       | F |
//  +---+-------------------------------+---+---+-------+---+
//                              |
//                              V
//  +---+-----------+---+---+---------------------------+---+
//  | I | 1... ...1 | I | F |                           | F | ( 6)
//  +---+-----------+---+---+---------------------------+---+
//  +---+-------------------------------+---+---+-------+---+
//  | I | 2...                     ...2 | I | F |       | F |
//  +---+-------------------------------+---+---+-------+---+
//                              |
//                              V
//  +---+-----------+---+---+---------------------------+---+
//  | I | 1... ...1 | I | F |                           | F | ( 7)
//  +---+-----------+---+---+---------------------------+---+
//  +---+-----------------------------------------------+---+
//  | F |                                               | F |
//  +---+-----------------------------------------------+---+

#include "crmalloc.h"
#include "nvstore.h"
#include <unistd.h>
#include <stdio.h>

/* From a pointer to a payload, get the block */
static inline struct block *payload_to_block(void *payload) {
    return (struct block*) (
        ((void *) payload) - offsetof(struct block, payload)
    );
}

/* Getter for right tag in block */
static inline union boundary_tag *block_right_tag(struct block *b) {
    return (union boundary_tag *) (
        ((void *) b) + sizeof(struct block) + b->tag.data.payload_size
    );
}

int main(int argc, char **argv) {

    nvstore_init("test_malloc.heap");

    // Step 1

    // Malloc a block that requires an entire page to store its payload + metadata
    size_t payload_1_size = PAGESIZE - METADATA_SIZE;
    char *payload_1 = cr_malloc(payload_1_size);
    struct block *block_1 = payload_to_block(payload_1);
    // Verify left tag is correct
    if (!block_1->tag.data.inuse) { 
        printf("(1) ERROR: Block incorrectly marked as free.\n"); 
    }
    if (block_1->tag.data.payload_size < payload_1_size) { 
        printf("(1) ERROR: Unexpected payload size.\n"); 
    }
    // Fill payload with 'a'
    char *p = payload_1;
    for (int i = 0; i < payload_1_size; i++) {
        *p = 'a';
        p++;
    }
    // Verify right tag is at expected place and is correct
    union boundary_tag *tag = (union boundary_tag *) p;
    if (!tag->data.inuse) { 
        printf("(1) ERROR: Block incorrectly marked as free.\n"); 
    }
    if (tag->data.payload_size < payload_1_size) { 
        printf("(1) ERROR: Unexpected payload size.\n"); 
    }

    // Step 2

    // Shrink block to 4 bytes
    payload_1_size = 4;
    payload_1 = cr_realloc(payload_1, payload_1_size);
    block_1 = payload_to_block(payload_1);
    // Verify left tag was updated properly
    if (!block_1->tag.data.inuse) { 
        printf("(2) ERROR: Block incorrectly marked as free.\n"); 
    }
    if (block_1->tag.data.payload_size < payload_1_size) { 
        printf("(2) ERROR: Unexpected payload size.\n"); 
    }
    // Verify right tag is in expected position and was updated properly
    p = payload_1 + payload_1_size;
    tag = (union boundary_tag *) p;
    if (!tag->data.inuse) { 
        printf("(2) ERROR: Block incorrectly marked as free.\n"); 
    }
    if (tag->data.payload_size < payload_1_size) { 
        printf("(2) ERROR: Unexpected payload size.\n"); 
    }
    // Verify left tag of free block is in expected position and was updated properly
    p += sizeof(union boundary_tag);
    struct block *block_free = (struct block *) p;
    if (block_free->tag.data.inuse) { 
        printf("(2) ERROR: Block incorrectly marked as inuse.\n"); 
    }
    if (block_free->tag.data.payload_size > PAGESIZE - payload_1_size - METADATA_SIZE*2) { 
        printf("(2) ERROR: Unexpected payload size.\n"); 
    }
    // Verify right tag of free block is in expected position and was updated properly
    p += block_free->tag.data.payload_size + sizeof(struct block);
    tag = (union boundary_tag *) p;
    if (tag->data.inuse) { 
        printf("(2) ERROR: Block incorrectly marked as inuse.\n"); 
    }
    if (tag->data.payload_size > PAGESIZE - payload_1_size - METADATA_SIZE*2) { 
        printf("(2) ERROR: Unexpected payload size.\n"); 
    }

    // Step 3

    nvstore_shutdown();

}
