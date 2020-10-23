#ifndef __NVADDRLIST_H__
#define __NVADDRLIST_H__

#include <stddef.h>

#define NVADDRLIST_INIT_POWER    12

/* simple arraylist used for storing a list of pages to checkpoint */
struct nvaddrlist
{
    void **addrs;
    size_t cap;
    size_t len;
};

/* helper functions for basic arraylist */
struct nvaddrlist *nvaddrlist_new(size_t power);
void nvaddrlist_delete(struct nvaddrlist *list);
void nvaddrlist_clear(struct nvaddrlist *list);
void nvaddrlist_insert(struct nvaddrlist *list, void *addr);

#endif
