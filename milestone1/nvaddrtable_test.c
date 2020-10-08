#include "nvaddrtable_test.h"
#include "nvaddrtable.h"
#include "nvblock.h"

#include <unistd.h>
#include <stddef.h>

#define SMALL_POWER     4
#define SMALL_SIZE      (1 << SMALL_POWER)

#define LARGE_POWER     16
#define LARGE_SIZE      (1 << LARGE_POWER)

const char *test_nvaddrtable_init()
{
    struct nvaddrtable *table;

    table = nvaddrtable_new(SMALL_POWER);

    if (table == NULL)
        return "Allocation failed.";

    nvaddrtable_delete(table);
    return NULL;
}

const char *test_nvaddrtable_basic_insertion()
{
    struct nvaddrtable *table;
    struct nvblock *block, *find;
    size_t i;

    block = nvblock_new(NULL, 1);
    table = nvaddrtable_new(SMALL_POWER);

    nvaddrtable_insert(table, block);
    
    find = nvaddrtable_find(table, block->pgstart);
    if (find != block)
        return "Searching for block using page start after insertion failed.";

    for (i = 0; i < (size_t)sysconf(_SC_PAGE_SIZE); i++)
    {
        find = nvaddrtable_find(table, block->pgstart + i);
        if (find != block)
            return "Searching for block using offset address failed.";
    }

    nvaddrtable_delete(table);
    nvblock_delete(block);

    return NULL;
}

const char *test_nvaddrtable_expansion()
{
    struct nvaddrtable *table;
    struct nvblock *blocks[LARGE_SIZE], *find;
    size_t i, j;

    table = nvaddrtable_new(SMALL_POWER);

    for (i = 0; i < LARGE_SIZE; i++)
    {
        blocks[i] = nvblock_new(NULL, 1);

        if (blocks[i] == NULL)
            return "Failure occurred in basic allocation.";

        nvaddrtable_insert(table, blocks[i]);
    }

    for (i = 0; i < LARGE_SIZE; i++)
    {
        for (j = 0; j < (size_t)sysconf(_SC_PAGE_SIZE); j += 1024)
        {
            find = nvaddrtable_find(table, blocks[i]->pgstart + j);
            if (find != blocks[i])
                return "Searching for block using offset address failed";
        }
    }

    nvaddrtable_delete(table);
    for (i = 0; i < LARGE_SIZE; i++)
        nvblock_delete(blocks[i]);

    return NULL;
}
