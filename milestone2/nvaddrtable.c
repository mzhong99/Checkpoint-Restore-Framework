#include "nvaddrtable.h"
#include "memcheck.h"

#include <unistd.h>
#include <assert.h>

#include <stdio.h>

static struct nventry *__nvaddrtable_find(struct nvaddrtable *table, void *key)
{
    size_t base, offset, hash, k;
    void *pgstart;

    for (k = 0; k < table->cap; k++)
    {
        pgstart = (void *)((uintptr_t)key & ~(sysconf(_SC_PAGE_SIZE) - 1));
        base = (size_t)pgstart / sysconf(_SC_PAGE_SIZE);
        offset = ((k * k) + k) >> 1;
        hash = (base + offset) % table->cap;

        if (table->entries[hash].key == pgstart)
            return &table->entries[hash];

        if (table->entries[hash].key == NULL)
            return &table->entries[hash];
    }

    return NULL;
}

struct nvaddrtable *nvaddrtable_new(size_t power)
{
    struct nvaddrtable *table = mc_malloc(sizeof(*table));

    table->nelem = 0;
    table->cap = 1 << power;

    table->entries = mc_calloc(table->cap, sizeof(*table->entries));

    return table;
}

void nvaddrtable_delete(struct nvaddrtable *table)
{
    mc_free(table->entries);
    mc_free(table);
}

void nvaddrtable_expand(struct nvaddrtable *table)
{
    struct nventry *oldentries;
    size_t oldcap, i;

    oldentries = table->entries;
    oldcap = table->cap;

    table->cap <<= 1;
    table->entries = mc_calloc(table->cap, sizeof(*table->entries));

    for (i = 0; i < oldcap; i++)
        if (oldentries[i].value != NULL)
            nvaddrtable_insert(table, oldentries[i].value);

    mc_free(oldentries);
}

void nvaddrtable_insert(struct nvaddrtable *table, struct nvblock *block)
{
    struct nventry *entry;
    void *pgstart;
    size_t pgidx;

    for (pgidx = 0; pgidx < block->npages; pgidx++)
    {
        /* expand at 70% load - this method prevents FPU divisions */
        if (10 * table->nelem > 7 * table->cap)
            nvaddrtable_expand(table);

        pgstart = block->pgstart + (pgidx * sysconf(_SC_PAGE_SIZE));
        entry = __nvaddrtable_find(table, pgstart);

        entry->key = pgstart;
        entry->value = block;
        table->nelem++;
    }
}

struct nvblock *nvaddrtable_find(struct nvaddrtable *table, void *addr)
{
    struct nventry *entry;

    entry = __nvaddrtable_find(table, addr);
    return entry->value;
}
