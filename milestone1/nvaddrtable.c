#include "nvaddrtable.h"

#include <unistd.h>
#include <assert.h>

#include <stdlib.h>

struct nvaddrtable *nvaddrtable_new(size_t power)
{
    struct nvaddrtable *table = malloc(sizeof(*table));

    table->nelem = 0;
    table->cap = 1 << power;

    table->values = calloc(table->cap, sizeof(*table->values));

    return table;
}

void nvaddrtable_delete(struct nvaddrtable *table)
{
    free(table->values);
    free(table);
}

size_t nvaddrtable_qhash(struct nvaddrtable *table, void *addr, size_t k)
{
    size_t base, offset, hash;
    void *pgstart;

    pgstart = (void *)((uintptr_t)addr & ~(sysconf(_SC_PAGE_SIZE) - 1));
    base = (size_t)pgstart / sysconf(_SC_PAGE_SIZE);
    offset = ((k * k) + k) >> 1;
    hash = (base + offset) % table->cap;

    return hash;
}

void nvaddrtable_expand(struct nvaddrtable *table)
{
    struct nvblock **oldvalues;
    size_t oldcap, i;

    oldcap = table->cap;
    oldvalues = table->values;

    table->cap <<= 1;
    table->values = calloc(table->cap, sizeof(*table->values));

    for (i = 0; i < oldcap; i++)
        if (oldvalues[i] != NULL)
            nvaddrtable_insert(table, oldvalues[i]);

    free(oldvalues);
}

void nvaddrtable_insert(struct nvaddrtable *table, struct nvblock *block)
{
    size_t k, hash;

    /* expand at 70% load - this method prevents divisions through the FPU */
    if (10 * table->nelem > 7 * table->cap)
        nvaddrtable_expand(table);

    for (k = 0; k < table->cap; k++)
    {
        hash = nvaddrtable_qhash(table, block->pgstart, k);

        if (table->values[hash] == NULL)
        {
            table->values[hash] = block;
            table->nelem++;
            break;
        }
    }

    assert(k < table->cap);
}

struct nvblock *nvaddrtable_find(struct nvaddrtable *table, void *addr)
{
    size_t k, hash;
    void *pgstart;

    pgstart = (void *)((uintptr_t)addr & ~(sysconf(_SC_PAGE_SIZE) - 1));

    for (k = 0; k < table->cap; k++)
    {
        hash = nvaddrtable_qhash(table, pgstart, k);

        if (table->values[hash] == NULL)
            return NULL;

        if (table->values[hash]->pgstart == pgstart)
            return table->values[hash];
    }

    return NULL;
}
