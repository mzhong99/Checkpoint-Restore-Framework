#include "vaddrtable_test.h"
#include "vaddrtable.h"
#include "vblock.h"

#include <unistd.h>
#include <stddef.h>

#define SMALL_POWER     4
#define SMALL_SIZE      (1 << SMALL_POWER)

#define LARGE_POWER     16
#define LARGE_SIZE      (1 << LARGE_POWER)

const char *test_vaddrtable_init()
{
    struct vaddrtable *table;

    table = vaddrtable_new(SMALL_POWER);

    if (table == NULL)
        return "Allocation failed.";

    vaddrtable_delete(table);
    return NULL;
}

const char *test_vaddrtable_basic_insertion()
{
    struct vaddrtable *table;
    struct vblock *block, *find;
    size_t i;

    block = vblock_new(NULL, 1, 0);
    table = vaddrtable_new(SMALL_POWER);

    vaddrtable_insert(table, block);
    
    find = vaddrtable_find(table, block->pgstart);
    if (find != block)
        return "Searching for block using page start after insertion failed.";

    for (i = 0; i < (size_t)sysconf(_SC_PAGE_SIZE); i++)
    {
        find = vaddrtable_find(table, block->pgstart + i);
        if (find != block)
            return "Searching for block using offset address failed.";
    }

    vaddrtable_delete(table);
    vblock_delete(block);

    return NULL;
}

const char *test_vaddrtable_expansion()
{
    struct vaddrtable *table;
    struct vblock *blocks[LARGE_SIZE], *find;
    size_t i, j;

    table = vaddrtable_new(SMALL_POWER);

    for (i = 0; i < LARGE_SIZE; i++)
    {
        blocks[i] = vblock_new(NULL, 1, 0);

        if (blocks[i] == NULL)
            return "Failure occurred in basic block allocation.";

        vaddrtable_insert(table, blocks[i]);
    }

    for (i = 0; i < LARGE_SIZE; i++)
    {
        for (j = 0; j < (size_t)sysconf(_SC_PAGE_SIZE); j += 1024)
        {
            find = vaddrtable_find(table, blocks[i]->pgstart + j);
            if (find != blocks[i])
                return "Searching for block using offset address failed";
        }
    }

    vaddrtable_delete(table);
    for (i = 0; i < LARGE_SIZE; i++)
        vblock_delete(blocks[i]);

    return NULL;
}

const char *test_vaddrtable_large_entries()
{
    struct vaddrtable *table;
    struct vblock *blocks[SMALL_SIZE], *find;
    size_t i, j;

    table = vaddrtable_new(SMALL_POWER);

    for (i = 0; i < SMALL_SIZE; i++)
    {
        blocks[i] = vblock_new(NULL, i + 1, 0);

        if (blocks[i] == NULL)
            return "Failure occurred in basic block allocation.";

        vaddrtable_insert(table, blocks[i]);
    }

    for (i = 0; i < SMALL_SIZE; i++)
    {
        for (j = 0; j < sysconf(_SC_PAGE_SIZE) * blocks[i]->npages; j += 1024)
        {
            find = vaddrtable_find(table, blocks[i]->pgstart + j);
            if (find != blocks[i])
                return "Searching for block using offset address failed.";
        }
    }

    vaddrtable_delete(table);
    for (i = 0; i < SMALL_SIZE; i++)
        vblock_delete(blocks[i]);

    return NULL;
}
