#ifndef __VADDRLIST_H__
#define __VADDRLIST_H__

#include <stddef.h>
#include "vtslist.h"

#define NVADDRLIST_INIT_POWER    4

/** 
 * A simple arraylist used for storing a list of addresses to checkpoint. Is NOT
 * thread-safe, and is volatile between executions of this program.
 */
struct vaddrlist
{
    void **addrs;
    size_t cap;
    size_t len;
};

/* Helper functions for basic arraylist */
struct vaddrlist *vaddrlist_new(size_t power);
void vaddrlist_delete(struct vaddrlist *list);

void vaddrlist_clear(struct vaddrlist *list);
void vaddrlist_insert(struct vaddrlist *list, void *addr);
void vaddrlist_insert_pages_of(struct vaddrlist *list, void *addr, size_t len);

#endif
