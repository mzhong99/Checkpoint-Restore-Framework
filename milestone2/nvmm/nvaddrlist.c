#include "nvaddrlist.h"
#include "memcheck.h"

struct nvaddrlist *nvaddrlist_new(size_t power)
{
    struct nvaddrlist *list;

    list = mc_malloc(sizeof(*list));

    list->len = 0;
    list->cap = 1 << power;
    list->addrs = mc_calloc(list->cap, sizeof(*list->addrs));

    return list;
}

void nvaddrlist_delete(struct nvaddrlist *list)
{
    mc_free(list->addrs);
    mc_free(list);
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
        list->addrs = mc_realloc(list->addrs, list->cap * sizeof(*list->addrs));
    }

    list->addrs[list->len++] = addr;
}
