#ifndef __NVADDRTABLE_H__
#define __NVADDRTABLE_H__

#include <stddef.h>
#include "nvblock.h"

#define NVADDRTABLE_DEFAULT_SIZE    (1 << 12)

/* simple hashtable used to map addresses back to the block that owns it */
struct nvaddrtable
{
    struct nvblock **values;
    size_t nelem;
    size_t cap;
};

/* helper functions for hashtable */
struct nvaddrtable *nvaddrtable_new(size_t power);
void nvaddrtable_delete(struct nvaddrtable *table);

size_t nvaddrtable_qhash(struct nvaddrtable *table, void *addr, size_t k);
void nvaddrtable_expand(struct nvaddrtable *table);
void nvaddrtable_insert(struct nvaddrtable *table, struct nvblock *block);
struct nvblock *nvaddrtable_find(struct nvaddrtable *table, void *addr);

#endif