#include "vaddrlist.h"
#include "memcheck.h"
#include <unistd.h>
#include <stdint.h>

struct vaddrlist *vaddrlist_new(size_t power)
{
    struct vaddrlist *list;

    list = mcmalloc(sizeof(*list));

    list->len = 0;
    list->cap = 1 << power;
    list->addrs = mccalloc(list->cap, sizeof(*list->addrs));

    return list;
}

void vaddrlist_delete(struct vaddrlist *list)
{
    mcfree(list->addrs);
    mcfree(list);
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
        list->addrs = mcrealloc(list->addrs, list->cap * sizeof(*list->addrs));
    }

    list->addrs[list->len++] = addr;
}

void vaddrlist_insert_pages_of(struct vaddrlist *list, void *addr, size_t len)
{
    void *page_iter, *addr_high;

    page_iter = (void *)((uintptr_t)addr & ~(sysconf(_SC_PAGE_SIZE) - 1));
    addr_high = addr + len;

    while (page_iter < addr_high)
    {
        vaddrlist_insert(list, page_iter);
        page_iter += sysconf(_SC_PAGE_SIZE);
    }
}