#include "vtsaddrtable.h"
#include "memcheck.h"

#include <unistd.h>
#include <assert.h>

#include <stdio.h>

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/
static void __vtsaddrtable_expand(struct vtsaddrtable *table);
static void __vtsaddrtable_insert(struct vtsaddrtable *table, 
                                  struct vblock *block);
static struct ventry *__vtsaddrtable_find(struct vtsaddrtable *table, 
                                          void *key);

/******************************************************************************/
/** Private Implementation -------------------------------------------------- */
/** Note: Private functions do NOT lock the data structure because the only   */
/**       context under which they can be called is under a locked context.   */
/******************************************************************************/

/** Finds the KV address-to-block pair for an address contained in a page.    */
static struct ventry *__vtsaddrtable_find(struct vtsaddrtable *table, void *key)
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

/** Expands the hash table when number of entries exceeds clustering limit    */
static void __vtsaddrtable_expand(struct vtsaddrtable *table)
{
    struct ventry *oldentries;
    size_t oldcap, i;

    oldentries = table->entries;
    oldcap = table->cap;

    table->cap <<= 1;
    table->entries = mc_calloc(table->cap, sizeof(*table->entries));

    for (i = 0; i < oldcap; i++)
        if (oldentries[i].value != NULL)
            __vtsaddrtable_insert(table, oldentries[i].value);

    mc_free(oldentries);
}

/** 
 * Inserts unsafely into table. Implementation separated from public insertion
 * in order to support expand-insert operations.
 */
static void __vtsaddrtable_insert(struct vtsaddrtable *table, struct vblock *block)
{
    struct ventry *entry;
    void *pgstart;
    size_t pgidx;

    for (pgidx = 0; pgidx < block->npages; pgidx++)
    {
        /* expand at 70% load - this method prevents FPU divisions */
        if (10 * table->nelem > 7 * table->cap)
            __vtsaddrtable_expand(table);

        pgstart = block->pgstart + (pgidx * sysconf(_SC_PAGE_SIZE));
        entry = __vtsaddrtable_find(table, pgstart);

        entry->key = pgstart;
        entry->value = block;
        table->nelem++;
    }
}

/******************************************************************************/
/** Public-Facing API ------------------------------------------------------- */
/******************************************************************************/
struct vtsaddrtable *vtsaddrtable_new(size_t power)
{
    struct vtsaddrtable *table = mc_malloc(sizeof(*table));

    table->nelem = 0;
    table->cap = 1 << power;

    table->entries = mc_calloc(table->cap, sizeof(*table->entries));
    pthread_rwlock_init(&table->lock, NULL);

    return table;
}

void vtsaddrtable_delete(struct vtsaddrtable *table)
{
    pthread_rwlock_destroy(&table->lock);

    mc_free(table->entries);
    mc_free(table);
}

void vtsaddrtable_insert(struct vtsaddrtable *table, struct vblock *block)
{
    pthread_rwlock_wrlock(&table->lock);
    __vtsaddrtable_insert(table, block);
    pthread_rwlock_unlock(&table->lock);
}

struct vblock *vtsaddrtable_find(struct vtsaddrtable *table, void *addr)
{
    struct ventry *entry;

    pthread_rwlock_rdlock(&table->lock);
    entry = __vtsaddrtable_find(table, addr);
    pthread_rwlock_unlock(&table->lock);

    return entry->value;
}
