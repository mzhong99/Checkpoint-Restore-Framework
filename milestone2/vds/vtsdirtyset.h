#ifndef __VTSADDRSET_H__
#define __VTSADDRSET_H__

#include <stdbool.h>
#include <pthread.h>

#include "list.h"

#define VDIRTYSET_SIZE      4096

struct vtsdirtyaddr
{
    void *ptr;
    struct list_elem elem;
    struct list_elem iter;
};

/**
 * Implementation of a thread-safe, volatile address set container which stores
 * dirty addresses. Uses a chained hashtable for internal representation. 
 * The Hash table is an array of thread-safe list. Supports simultaneous 
 * insertions and removals, but each removal MUST be conducted by just ONE 
 * thread. 
 * 
 * We don't use the vtslist here simply because the operations we need to 
 * support are different enough from the thread-safe operations provided by the
 * vtslist.
 */
struct vtsdirtyset
{
    struct list buckets[VDIRTYSET_SIZE];
    struct list iterlist;
    pthread_mutex_t lock;
};

struct vtsdirtyset *vtsdirtyset_new();
struct vtsdirtyset *vtsdirtyset_copy(struct vtsdirtyset *base);
void vtsdirtyset_delete(struct vtsdirtyset *set);

void vtsdirtyset_insert(struct vtsdirtyset *set, void *addr);
void *vtsdirtyset_remove(struct vtsdirtyset *set, void *addr);
void *vtsdirtyset_remove_any(struct vtsdirtyset *set);

#endif