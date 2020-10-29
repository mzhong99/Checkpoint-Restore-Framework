#include "vthreadtable.h"
#include "macros.h"

struct vthreadtable *vthreadtable_new()
{
    size_t i;
    struct vthreadtable *table;

    table = mc_malloc(sizeof(*table));
    table->nelem = 0;

    for (i = 0; i < VTHREADTABLE_SIZE; i++)
        vtslist_init(&table->data[i]);

    return table;
}

void vthreadtable_delete(struct vthreadtable *table)
{
    size_t i;
    for (i = 0; i < VTHREADTABLE_SIZE; i++)
        vtslist_cleanup(&table->data[i]);

    mc_free(table);
}

void vthreadtable_insert(struct vthreadtable *table, struct crthread *handle)
{
    struct vtslist *vtslist;
    size_t hash;
    
    hash = handle->ptid % VTHREADTABLE_SIZE;
    vtslist = &table->data[hash];
    vtslist_push_back(vtslist, &handle->vtselem);
}

struct crthread *vthreadtable_find(struct vthreadtable *table, pthread_t id)
{
    struct vtslist *vtslist;

    struct vtslist_elem *vtselem;
    struct list_elem *elem;
    struct crthread *handle, *fetch = NULL;

    size_t hash;

    hash = id % VTHREADTABLE_SIZE;
    vtslist = &table->data[hash];

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

struct crthread *vthreadtable_remove(struct vthreadtable *table, pthread_t id)
{
    struct crthread *fetch;

    fetch = vthreadtable_find(table, id);
    if (fetch != NULL)
        vtslist_remove(&fetch->vtselem);

    return fetch;
}