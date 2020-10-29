#ifndef __VADDRTABLE_H__
#define __VADDRTABLE_H__

#include <stddef.h>
#include "vblock.h"

#define NVADDRTABLE_INIT_POWER  12

struct ventry
{
    void *key;
    struct vblock *value;
};

/* simple hashtable used to map addresses back to the block that owns it */
struct vaddrtable
{
    struct ventry *entries;
    size_t nelem;
    size_t cap;
};

/* helper functions for hashtable */
struct vaddrtable *vaddrtable_new(size_t power);
void vaddrtable_delete(struct vaddrtable *table);

void vaddrtable_expand(struct vaddrtable *table);
void vaddrtable_insert(struct vaddrtable *table, struct vblock *block);
struct vblock *vaddrtable_find(struct vaddrtable *table, void *addr);

#endif
