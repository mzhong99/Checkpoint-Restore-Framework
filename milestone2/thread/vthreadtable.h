#ifndef __VTHREADTABLE_H__
#define __VTHREADTABLE_H__

#include "memcheck.h"
#include "crthread.h"

#include "tslist.h"

#include <sys/types.h>
#include <stddef.h>

#define VTHREADTABLE_SIZE       256

/** used to convert pthread_t ids into crthread handles through lookup */
struct vthreadtable
{
    struct vtslist data[VTHREADTABLE_SIZE];
    size_t nelem;
};

struct vthreadtable *vthreadtable_new();
void vthreadtable_delete(struct vthreadtable *table);

void vthreadtable_insert(struct vthreadtable *table, struct crthread *handle);
struct crthread *vthreadtable_find(struct vthreadtable *table, pthread_t id);
struct crthread *vthreadtable_remove(struct vthreadtable *table, pthread_t id);

#endif