#include "vblock_test.h"
#include "vblock.h"

#include <unistd.h>
#include <stdint.h>
#include <stddef.h>

#define SMALL_NUM_PAGES     4
#define LARGE_NUM_PAGES     16

#define NUM_TRIALS          10

const char *test_vblock_basic()
{
    struct vblock *block;
    void *prevaddr;
    int32_t i;

    block = vblock_new(NULL, SMALL_NUM_PAGES, 0);

    if (block == NULL)
        return "Failed on first allocation.";

    if (block->pgstart == NULL)
        return "Block data address should not be NULL.";

    for (i = 0; i < sysconf(_SC_PAGE_SIZE) * SMALL_NUM_PAGES; i++)
        ((uint8_t *)block->pgstart)[i] = (uint8_t)i;

    prevaddr = block->pgstart;
    vblock_delete(block);

    block = vblock_new(prevaddr, SMALL_NUM_PAGES, 0);

    if (block == NULL)
        return "Failed on second allocation.";

    if (block->pgstart == NULL)
        return "Block data address should not be NULL.";

    if (block->pgstart != prevaddr)
        return "Block address should be the same as previous address";

    for (i = 0; i < sysconf(_SC_PAGE_SIZE) * SMALL_NUM_PAGES; i++)
        ((uint8_t *)block->pgstart)[i] = (uint8_t)i;

    vblock_delete(block);

    return NULL;
}

const char *test_vblock_advanced()
{
    size_t i, j;
    struct vblock *blocks[NUM_TRIALS];
    void *prevaddrs[NUM_TRIALS];

    for (i = 0; i < NUM_TRIALS; i++)
    {
        blocks[i] = vblock_new(NULL, LARGE_NUM_PAGES * (i + 1), 0);

        if (blocks[i] == NULL)
            return "Failed on allocation";

        for (j = 0; j < sysconf(_SC_PAGE_SIZE) * LARGE_NUM_PAGES * (i + 1); j++)
            ((uint8_t *)blocks[i]->pgstart)[j] = (uint8_t)j;
    }

    for (i = 0; i < NUM_TRIALS; i++)
    {
        prevaddrs[i] = blocks[i]->pgstart;
        vblock_delete(blocks[i]);
    }

    for (i = 0; i < NUM_TRIALS; i++)
    {
        blocks[i] = vblock_new(prevaddrs[i], LARGE_NUM_PAGES * (i + 1), 0);
        
        if (blocks[i] == NULL)
            return "Failed on allocation";

        if (blocks[i]->pgstart != prevaddrs[i])
            return "Page address does not match previous address";

        for (j = 0; j < sysconf(_SC_PAGE_SIZE) * LARGE_NUM_PAGES * (i + 1); j++)
            ((uint8_t *)blocks[i]->pgstart)[j] = (uint8_t)j;
    }

    for (i = 0; i < NUM_TRIALS; i++)
        vblock_delete(blocks[i]);

    return NULL;
}
