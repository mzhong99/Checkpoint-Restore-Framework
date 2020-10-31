#ifndef __TSLIST_H__
#define __TSLIST_H__

#include <pthread.h>
#include "list.h"

/**
 * Implementation of a volatile, thread-safe linked list, based on the original
 * list.h linked list. Each operation presented is thread-safe, but the behavior
 * of this list is undefined after a restoration.
 */

/** A volatile, thread-safe list element. */
struct vtslist_elem
{
    struct vtslist *owner;
    struct list_elem elem;
};

/** A volatile, thread-safe list. */
struct vtslist
{
    struct list list;
    pthread_mutex_t lock;
    pthread_cond_t haselems;
};

/** Initialization and cleanup of list (cleans up locks) */
void vtslist_init(struct vtslist *vtslist);
void vtslist_init_locks_only(struct vtslist *vtslist);
void vtslist_cleanup(struct vtslist *vtslist);

/** Insertion operations */
void vtslist_push_back(struct vtslist *vtslist, struct vtslist_elem *vtselem);
void vtslist_push_front(struct vtslist *vtslist, struct vtslist_elem *vtselem);

/** Simple polling operations - these block if the list has no elements */
struct vtslist_elem *vtslist_pop_back(struct vtslist *vtslist);
struct vtslist_elem *vtslist_pop_front(struct vtslist *vtslist);

/** Simple non-blocking poll operations */
struct vtslist_elem *vtslist_try_pop_back(struct vtslist *vtslist);
struct vtslist_elem *vtslist_try_pop_front(struct vtslist *vtslist);

/** Removes an element directly from the middle of a thread-safe list */
void vtslist_remove(struct vtslist_elem *vtselem);

#endif