#include "nvaddrlist_test.h"
#include "nvaddrlist.h"

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#define SMALL_LIST_POWER    6
#define SMALL_LIST_SIZE     (2 << SMALL_LIST_POWER)

#define LARGE_LIST_POWER    16
#define LARGE_LIST_SIZE     (2 << LARGE_LIST_POWER)

#define NUM_TRIALS          5

const char *test_nvaddrlist_init()
{
    struct nvaddrlist *list;

    list = nvaddrlist_new(1);
    if (list == NULL)
        return "Default construction failed.";

    nvaddrlist_delete(list);
    return NULL;
}

const char *test_nvaddrlist_basic_insertion()
{
    struct nvaddrlist *list;
    void *addrs[SMALL_LIST_SIZE];
    int i;

    list = nvaddrlist_new(SMALL_LIST_POWER);

    if (list->len != 0)
        return "Length of list should be zero at the very start.";

    for (i = 0; i < SMALL_LIST_SIZE; i++)
    {
        addrs[i] = (void *)(uintptr_t)rand();
        nvaddrlist_insert(list, addrs[i]);
    }

    if (list->len != SMALL_LIST_SIZE)
        return "Length of list is wrong after insertion (first round).";

    for (i = 0; i < SMALL_LIST_SIZE; i++)
        if (addrs[i] != list->addrs[i])
            return "After insertion (first round), list contents are wrong.";

    nvaddrlist_clear(list);

    if (list->len != 0)
        return "Length of list should be 0 after clear";
    
    for (i = 0; i < SMALL_LIST_SIZE; i++)
    {
        addrs[i] = (void *)(uintptr_t)rand();
        nvaddrlist_insert(list, addrs[i]);
    }

    if (list->len != SMALL_LIST_SIZE)
        return "Length of list is wrong after insertion (second round).";

    for (i = 0; i < SMALL_LIST_SIZE; i++)
        if (addrs[i] != list->addrs[i])
            return "After insertion (second round), list contents are wrong.";

    nvaddrlist_delete(list);

    return NULL;
}

const char *test_nvaddrlist_large_insertion()
{
    struct nvaddrlist *list;
    void *addrs[LARGE_LIST_SIZE];
    int i, ntrials;

    list = nvaddrlist_new(1);

    for (ntrials = 0; ntrials < NUM_TRIALS; ntrials++)
    {
        nvaddrlist_clear(list);
        for (i = 0; i < LARGE_LIST_SIZE; i++)
        {
            addrs[i] = (void *)(uintptr_t)rand();
            nvaddrlist_insert(list, addrs[i]);
        }

        for (i = 0; i < SMALL_LIST_SIZE; i++)
            if (addrs[i] != list->addrs[i])
                return "After insertion, list contents are wrong.";
    }

    nvaddrlist_delete(list);

    return NULL;
}
