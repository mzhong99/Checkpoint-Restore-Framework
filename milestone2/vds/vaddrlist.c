#include "vaddrlist.h"
#include "memcheck.h"

struct vaddrlist *vaddrlist_new(size_t power)
{
    struct vaddrlist *list;

    list = mc_malloc(sizeof(*list));

    list->len = 0;
    list->cap = 1 << power;
    list->addrs = mc_calloc(list->cap, sizeof(*list->addrs));

    return list;
}

void vaddrlist_delete(struct vaddrlist *list)
{
    mc_free(list->addrs);
    mc_free(list);
}

void vaddrlist_clear(struct vaddrlist *list)
{
    list->len = 0;
}

void vaddrlist_insert(struct vaddrlist *list, void *addr)
{
    if (list->len == list->cap)
    {
        list->cap <<= 1;
        list->addrs = mc_realloc(list->addrs, list->cap * sizeof(*list->addrs));
    }

    list->addrs[list->len++] = addr;
}
