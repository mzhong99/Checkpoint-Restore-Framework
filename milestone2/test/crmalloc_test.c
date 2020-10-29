#include "crmalloc_test.h"
#include "crheap.h"

#include <stdlib.h>
#include <unistd.h>

#define NTRIALS 20

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

/******************************************************************************/
/** Basic functionality Unit Tests - imported from old non-freelist crmalloc  */
/******************************************************************************/

const char *test_crmalloc_simple()
{
    int i, *data;

    crheap_init("test_crmalloc_simple.heap");
    data = crmalloc(sizeof(int) * sysconf(_SC_PAGE_SIZE));
    
    for (i = 0; i < sysconf(_SC_PAGE_SIZE); i++)
        data[i] = i;

    for (i = 0; i < sysconf(_SC_PAGE_SIZE); i++)
        if (data[i] != i)
            return "Data written to a crmalloc() block is incorrect";

    crfree(data);
    crheap_shutdown();

    return NULL;
}

const char *test_crmalloc_complex()
{
    int i, trial, npages, *data;

    crheap_init("test_crmalloc_complex.heap");

    for (trial = 0; trial < NTRIALS; trial++)
    {
        npages = (abs(rand()) % (trial + 1)) + 1;
        data = crmalloc(sizeof(int) * sysconf(_SC_PAGE_SIZE) * npages);

        for (i = 0; i < sysconf(_SC_PAGE_SIZE) * npages; i++)
            data[i] = i;

        for (i = 0; i < sysconf(_SC_PAGE_SIZE) * npages; i++)
            if (data[i] != i)
                return "Data written to a crmalloc() block is incorrect";

        crfree(data);
    }

    crheap_shutdown();
    return NULL;
}

/**
 * Tests by allocating NTRIAL arrays of integers, filling them with data, then
 * shutting and restarting the heap system. The data needs to still be correct
 * after a crheap_shutdown() followed by a crheap_init() reinitialization.
 */
const char *test_crmalloc_recovery()
{
    int i, trial, elapse, npages[NTRIALS], *data[NTRIALS];

    /* ---------------------------------------------------------------------- */
    /* Step 1: Preallocate data to populate heap                              */
    /* ---------------------------------------------------------------------- */
    crheap_init("test_crmalloc_recovery.heap");

    for (trial = 0; trial < NTRIALS; trial++)
    {
        npages[trial] = (abs(rand()) % (trial + 1)) + 1;
        data[trial] = crmalloc(
            sizeof(int) * sysconf(_SC_PAGE_SIZE) * npages[trial]);

        for (i = 0; i < sysconf(_SC_PAGE_SIZE) * npages[trial]; i++)
            data[trial][i] = i;
    }

    crheap_shutdown();

    /* ---------------------------------------------------------------------- */
    /* Step 2: Continuously turn on, validate, free, and reallocate data      */
    /* ---------------------------------------------------------------------- */
    for (elapse = 0; elapse < NTRIALS; elapse++)
    {
        /* Before beginning one round (one elapse) of tests, first we call
         * a reinitialization routine to rebuild heap integrity. */
        crheap_init("test_crmalloc_recovery.heap");

        /* First, for each block, validate that the data is correct */
        for (trial = 0; trial < NTRIALS; trial++)
            for (i = 0; i < sysconf(_SC_PAGE_SIZE) * npages[trial]; i++)
                if (data[trial][i] != i)
                    return "Data in a crmalloc() block after shutdown and "
                           "reinitialization with same heap file is incorrect";
                
        /* Next, free each of the old data blocks */
        for (trial = 0; trial < NTRIALS; trial++)
            crfree(data[trial]);
        
        /* Finally, call crmalloc() again to attempt to allocate and repopulate.
         * everything. We do this in a separate loop passthrough in order to
         * attempt to stress-test the free-blocks list in the allocator. */
        for (trial = 0; trial < NTRIALS; trial++)
        {
            npages[trial] = (abs(rand()) % (trial + 1)) + 1;
            data[trial] = crmalloc(
                sizeof(int) * sysconf(_SC_PAGE_SIZE) * npages[trial]);

            for (i = 0; i < sysconf(_SC_PAGE_SIZE) * npages[trial]; i++)
                data[trial][i] = i;
        }

        /* After one successful round of testing, we need to teardown the heap
         * to show that shutdowns and restarts consistently work. */
        crheap_shutdown();
    }

    /* ---------------------------------------------------------------------- */
    /* Step 3: Free everything, one last time                                 */
    /* ---------------------------------------------------------------------- */
    crheap_init("test_crmalloc_recovery.heap");
    for (trial = 0; trial < NTRIALS; trial++)
        crfree(data[trial]);
    crheap_shutdown();

    /* Test is successful - notify caller as such! */
    return NULL;
}

/******************************************************************************/
/** Thorough unit testing with inner data structure validation -------------- */
/******************************************************************************/

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

const char *test_crmalloc_integration()
{
    size_t payload_1_size, i;
    char *payload_1, *p;
    struct block *block_1, *block_free;
    union boundary_tag *tag;

    crheap_init("test_crmalloc_integration.heap");

    // Step 1

    // Malloc a block that requires an entire page to store its payload + metadata
    payload_1_size = PAGESIZE - METADATA_SIZE;
    payload_1 = crmalloc(payload_1_size);
    block_1 = payload_to_block(payload_1);
    // Verify left tag is correct
    if (!block_1->tag.data.inuse) { 
        return "(1) ERROR: Block incorrectly marked as free.\n"; 
    }
    if (block_1->tag.data.payload_size < payload_1_size) { 
        return "(1) ERROR: Unexpected payload size.\n"; 
    }
    // Fill payload with 'a'
    p = payload_1;
    for (i = 0; i < payload_1_size; i++) {
        *p = 'a';
        p++;
    }
    // Verify right tag is at expected place and is correct
    tag = (union boundary_tag *) p;
    if (!tag->data.inuse) { 
        return "(1) ERROR: Block incorrectly marked as free.\n"; 
    }
    if (tag->data.payload_size < payload_1_size) { 
        return "(1) ERROR: Unexpected payload size.\n";
    }

    // Step 2

    // Shrink block to 4 bytes
    payload_1_size = 4;
    payload_1 = crrealloc(payload_1, payload_1_size);
    block_1 = payload_to_block(payload_1);
    // Verify left tag was updated properly
    if (!block_1->tag.data.inuse) { 
        return "(2) ERROR: Block incorrectly marked as free.\n"; 
    }
    if (block_1->tag.data.payload_size < payload_1_size) { 
        return "(2) ERROR: Unexpected payload size.\n"; 
    }
    // Verify right tag is in expected position and was updated properly
    p = payload_1 + payload_1_size;
    tag = (union boundary_tag *) p;
    if (!tag->data.inuse) { 
        return "(2) ERROR: Block incorrectly marked as free.\n"; 
    }
    if (tag->data.payload_size < payload_1_size) { 
        return "(2) ERROR: Unexpected payload size.\n"; 
    }
    // Verify left tag of free block is in expected position and was updated properly
    p += sizeof(union boundary_tag);
    block_free = (struct block *) p;
    if (block_free->tag.data.inuse) { 
        return "(2) ERROR: Block incorrectly marked as inuse.\n"; 
    }
    if (block_free->tag.data.payload_size > PAGESIZE - payload_1_size - METADATA_SIZE*2) { 
        return "(2) ERROR: Unexpected payload size.\n"; 
    }
    // Verify right tag of free block is in expected position and was updated properly
    p += block_free->tag.data.payload_size + sizeof(struct block);
    tag = (union boundary_tag *) p;
    if (tag->data.inuse) { 
        return "(2) ERROR: Block incorrectly marked as inuse.\n";
    }
    if (tag->data.payload_size > PAGESIZE - payload_1_size - METADATA_SIZE*2) { 
        return "(2) ERROR: Unexpected payload size.\n";
    }

    // Step 3

    crheap_shutdown();

    return NULL;
}
