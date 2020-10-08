#include "nvaddrlist.h"
#include <stdlib.h>

struct nvaddrlist *nvaddrlist_new(size_t power)
{
    struct nvaddrlist *list;

    list = malloc(sizeof(*list));

    list->len = 0;
    list->cap = 1 << power;
    list->addrs = calloc(list->cap, sizeof(*list->addrs));

    return list;
}

void nvaddrlist_delete(struct nvaddrlist *list)
{
    free(list->addrs);
    free(list);
}

void nvaddrlist_clear(struct nvaddrlist *list)
{
    list->len = 0;
}

void nvaddrlist_insert(struct nvaddrlist *list, void *addr)
{
    if (list->len == list->cap)
    {
        list->cap <<= 1;
        list->addrs = realloc(list->addrs, list->cap * sizeof(*list->addrs));
    }

    list->addrs[list->len++] = addr;
}
