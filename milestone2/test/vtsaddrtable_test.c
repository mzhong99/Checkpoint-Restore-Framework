#include "vtsaddrtable_test.h"
#include "vtsaddrtable.h"
#include "vblock.h"

#include <unistd.h>
#include <stddef.h>

#define SMALL_POWER     4
#define SMALL_SIZE      (1 << SMALL_POWER)

#define LARGE_POWER     16
#define LARGE_SIZE      (1 << LARGE_POWER)

const char *test_vtsaddrtable_init()
{
    struct vtsaddrtable *table;

    table = vtsaddrtable_new(SMALL_POWER);

    if (table == NULL)
        return "Allocation failed.";

    vtsaddrtable_delete(table);
    return NULL;
}

const char *test_vtsaddrtable_basic_insertion()
{
    struct vtsaddrtable *table;
    struct vblock *block, *find;
    size_t i;

    block = vblock_new(NULL, 1, 0);
    table = vtsaddrtable_new(SMALL_POWER);

    vtsaddrtable_insert(table, block);
    
    find = vtsaddrtable_find(table, block->pgstart);
    if (find != block)
        return "Searching for block using page start after insertion failed.";

    for (i = 0; i < (size_t)sysconf(_SC_PAGE_SIZE); i++)
    {
        find = vtsaddrtable_find(table, block->pgstart + i);
        if (find != block)
            return "Searching for block using offset address failed.";
    }

    vtsaddrtable_delete(table);
    vblock_delete(block);

    return NULL;
}

const char *test_vtsaddrtable_expansion()
{
    struct vtsaddrtable *table;
    struct vblock *blocks[LARGE_SIZE], *find;
    size_t i, j;

    table = vtsaddrtable_new(SMALL_POWER);

    for (i = 0; i < LARGE_SIZE; i++)
    {
        blocks[i] = vblock_new(NULL, 1, 0);

        if (blocks[i] == NULL)
            return "Failure occurred in basic block allocation.";

        vtsaddrtable_insert(table, blocks[i]);
    }

    for (i = 0; i < LARGE_SIZE; i++)
    {
        for (j = 0; j < (size_t)sysconf(_SC_PAGE_SIZE); j += 1024)
        {
            find = vtsaddrtable_find(table, blocks[i]->pgstart + j);
            if (find != blocks[i])
                return "Searching for block using offset address failed";
        }
    }

    vtsaddrtable_delete(table);
    for (i = 0; i < LARGE_SIZE; i++)
        vblock_delete(blocks[i]);

    return NULL;
}

const char *test_vtsaddrtable_large_entries()
{
    struct vtsaddrtable *table;
    struct vblock *blocks[SMALL_SIZE], *find;
    size_t i, j;

    table = vtsaddrtable_new(SMALL_POWER);

    for (i = 0; i < SMALL_SIZE; i++)
    {
        blocks[i] = vblock_new(NULL, i + 1, 0);

        if (blocks[i] == NULL)
            return "Failure occurred in basic block allocation.";

        vtsaddrtable_insert(table, blocks[i]);
    }

    for (i = 0; i < SMALL_SIZE; i++)
    {
        for (j = 0; j < sysconf(_SC_PAGE_SIZE) * blocks[i]->npages; j += 1024)
        {
            find = vtsaddrtable_find(table, blocks[i]->pgstart + j);
            if (find != blocks[i])
                return "Searching for block using offset address failed.";
        }
    }

    vtsaddrtable_delete(table);
    for (i = 0; i < SMALL_SIZE; i++)
        vblock_delete(blocks[i]);

    return NULL;
}
