#ifndef __VADDRLIST_H__
#define __VADDRLIST_H__

#include <stddef.h>

#define NVADDRLIST_INIT_POWER    4

/* simple arraylist used for storing a list of pages to checkpoint */
struct vaddrlist
{
    void **addrs;
    size_t cap;
    size_t len;
};

/* helper functions for basic arraylist */
struct vaddrlist *vaddrlist_new(size_t power);
void vaddrlist_delete(struct vaddrlist *list);
void vaddrlist_clear(struct vaddrlist *list);
void vaddrlist_insert(struct vaddrlist *list, void *addr);

#endif
