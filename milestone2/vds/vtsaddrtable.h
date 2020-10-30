#ifndef __VADDRTABLE_H__
#define __VADDRTABLE_H__

#include <pthread.h>
#include <stddef.h>
#include "vblock.h"

#define NVADDRTABLE_INIT_POWER  12

/**
 * A hash table used to map raw addresses to the blocks from which the pages 
 * begin. Because insertions to this table occur during ALL invocations of
 * [nvstore_allocpage()], the outward-facing functions of this hashtable NEED
 * to be thread-safe.
 * 
 * This data structure is volatile, but thread-safe.
 */

/** Hash table entry container */
struct ventry
{
    void *key;
    struct vblock *value;
};

/** Baseline hash table data structure */
struct vtsaddrtable
{
    struct ventry *entries;
    size_t nelem;
    size_t cap;

    pthread_rwlock_t lock;
};

/* construction and deletion functions for hash table */
struct vtsaddrtable *vtsaddrtable_new(size_t power);
void vtsaddrtable_delete(struct vtsaddrtable *table);

/* insert and find operations */
void vtsaddrtable_insert(struct vtsaddrtable *table, struct vblock *block);
struct vblock *vtsaddrtable_find(struct vtsaddrtable *table, void *addr);

#endif
