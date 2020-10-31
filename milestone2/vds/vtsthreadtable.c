#include "vtsthreadtable.h"
#include "macros.h"
#include "vtslist.h"

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/
#define VTHREADTABLE_SIZE       256

/* Hash table - array of thread-safe list. Supports simultaneous insertions 
 * and removals, but each removal MUST be conducted by just ONE thread. */
static struct vtslist table[VTHREADTABLE_SIZE];

/******************************************************************************/
/** Public-Facing API ------------------------------------------------------- */
/******************************************************************************/
void vtsthreadtable_init()
{
    size_t i;
    for (i = 0; i < VTHREADTABLE_SIZE; i++)
        vtslist_init(&table[i]);
}

void vtsthreadtable_cleanup()
{
    size_t i;
    for (i = 0; i < VTHREADTABLE_SIZE; i++)
        vtslist_cleanup(&table[i]);
}

void vtsthreadtable_insert(struct crthread *handle)
{
    struct vtslist *vtslist;
    size_t hash;
    
    hash = handle->ptid % VTHREADTABLE_SIZE;
    vtslist = &table[hash];
    vtslist_push_back(vtslist, &handle->vtselem);
}

struct crthread *vtsthreadtable_find(pthread_t id)
{
    struct vtslist *vtslist;

    struct vtslist_elem *vtselem;
    struct list_elem *elem;
    struct crthread *handle, *fetch = NULL;

    size_t hash;

    hash = id % VTHREADTABLE_SIZE;
    vtslist = &table[hash];

    pthread_mutex_lock(&vtslist->lock);

    elem = list_begin(&vtslist->list);
    while (elem != list_end(&vtslist->list))
    {
        vtselem = container_of(elem, struct vtslist_elem, elem);
        handle = container_of(vtselem, struct crthread, vtselem);

        if (handle->ptid == id)
        {
            fetch = handle;
            break;
        }

        elem = list_next(elem);
    }

    pthread_mutex_unlock(&vtslist->lock);

    return fetch;
}

struct crthread *vtsthreadtable_remove(pthread_t id)
{
    struct crthread *fetch;

    fetch = vtsthreadtable_find(id);
    if (fetch != NULL)
        vtslist_remove(&fetch->vtselem);

    return fetch;
}