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
#include <unistd.h>
#include <stdio.h>

/* From a pointer to a payload, get the block */
static inline struct block *payload_to_block(void *payload) {
    return (struct block*) (
        ((void *) payload) - offsetof(struct block, payload)
    );
}

int main(int argc, char **argv) {

    // Step 1
    size_t payload_1_size = (size_t) sysconf(_SC_PAGESIZE);
    char *payload_1 = cr_malloc(payload_1_size);
    struct block *block_1 = payload_to_block(payload_1);
    if (!block_1->tag.data.inuse) { 
        printf("(1) ERROR: Block incorrectly marked as free.\n"); 
    }
    if (block_1->tag.data.payload_size < payload_1_size) { 
        printf("(1) ERROR: Block payload too small.\n"); 
    }
    char *p = payload_1;
    for (int i = 0; i < payload_1_size; i++) {
        *p = 'a';
        p++;
    }
    union boundary_tag *right_tag_1 = (union boundary_tag *) p;
    if (!right_tag_1->data.inuse) { 
        printf("(1) ERROR: Block incorrectly marked as free.\n"); 
    }
    if (right_tag_1->data.payload_size < payload_1_size) { 
        printf("(1) ERROR: Block payload too small.\n"); 
    }

}
