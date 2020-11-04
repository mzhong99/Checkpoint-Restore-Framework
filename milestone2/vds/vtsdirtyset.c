#include "vtsdirtyset.h"
#include "memcheck.h"
#include "ptr_hash.h"
#include "macros.h"

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/
static struct vtsdirtyaddr *__vtsdirtyaddr_new(void *key);
static void __vtsdirtyaddr_delete(struct vtsdirtyaddr *entry);

static struct vtsdirtyaddr *
__vtsdirtyset_find(struct vtsdirtyset *set, void *key);

static void __vtsdirtyset_insert(struct vtsdirtyset *set, void *key);
static void *__vtsdirtyset_remove(struct vtsdirtyset *set, void *key);
static void *__vtsdirtyset_remove_any(struct vtsdirtyset *set);

/******************************************************************************/
/** Private Implementation -------------------------------------------------- */
/** Note: Private functions do NOT lock the data structure because the only   */
/**       context under which they can be called is under a locked context.   */
/******************************************************************************/

/** Creates a new dirty address entry to be inserted in the table */
static struct vtsdirtyaddr *__vtsdirtyaddr_new(void *key)
{
    struct vtsdirtyaddr *entry;
    
    entry = mcmalloc(sizeof(*entry));
    entry->ptr = key;
    
    return entry;
}

/** Deletes the entry. The entry should NOT be in any lists before deletion.  */
static void __vtsdirtyaddr_delete(struct vtsdirtyaddr *entry)
{
    mcfree(entry);
}

/** Finds an entry (unsafely) corresponding to the address provided.          */
static struct vtsdirtyaddr *
__vtsdirtyset_find(struct vtsdirtyset *set, void *key)
{
    struct vtsdirtyaddr *entry;
    struct list_elem *bucket_it;
    size_t hash;

    hash = ptr_hash(key) % VDIRTYSET_SIZE;
    bucket_it = list_begin(&set->buckets[hash]);

    while (bucket_it != list_end(&set->buckets[hash]))
    {
        entry = list_entry(bucket_it, struct vtsdirtyaddr, elem);

        if (entry->ptr == key)
            return entry;

        bucket_it = list_next(bucket_it);
    }

    return NULL;
}

/** Inserts (unsafely) an address into the set                                */
static void __vtsdirtyset_insert(struct vtsdirtyset *set, void *key)
{
    struct vtsdirtyaddr *entry;
    size_t hash;

    entry = __vtsdirtyset_find(set, key);

    if (entry != NULL) 
        return;

    hash = ptr_hash(key) % VDIRTYSET_SIZE;
    entry = __vtsdirtyaddr_new(key);

    list_push_back(&set->buckets[hash], &entry->elem);
    list_push_back(&set->iterlist, &entry->iter);
}

/** Removes (unsafely) the specified address, if it exists.                   */
static void *__vtsdirtyset_remove(struct vtsdirtyset *set, void *key)
{
    struct vtsdirtyaddr *entry;

    entry = __vtsdirtyset_find(set, key);

    if (entry == NULL)
        return NULL;

    list_remove(&entry->elem);
    list_remove(&entry->iter);
    __vtsdirtyaddr_delete(entry);

    return key;
}

/** Removes (unsafely) ANY element from the hash table, if it exists          */
static void *__vtsdirtyset_remove_any(struct vtsdirtyset *set)
{
    struct list_elem *iter;
    struct vtsdirtyaddr *entry;
    void *key;

    if (list_empty(&set->iterlist))
        return NULL;

    iter = list_pop_front(&set->iterlist);

    entry = list_entry(iter, struct vtsdirtyaddr, iter);
    key = entry->ptr;

    list_remove(&entry->elem);
    __vtsdirtyaddr_delete(entry);

    return key;
}

/******************************************************************************/
/** Public-Facing API ------------------------------------------------------- */
/******************************************************************************/

/** Constructs a new dirty address set */
struct vtsdirtyset *vtsdirtyset_new()
{
    struct vtsdirtyset *set;
    size_t i;

    set = mcmalloc(sizeof(*set));

    pthread_mutex_init(&set->lock, NULL);
    list_init(&set->iterlist);

    for (i = 0; i < VDIRTYSET_SIZE; i++)
        list_init(&set->buckets[i]);

    return set;
}

/** Copies a dirty set which must be also be freed using vtsdirtyset_delete() */
struct vtsdirtyset *vtsdirtyset_copy(struct vtsdirtyset *base)
{
    struct vtsdirtyaddr *entry;
    struct list_elem *elem;

    struct vtsdirtyset *copy = vtsdirtyset_new();

    pthread_mutex_lock(&base->lock);

    elem = list_begin(&base->iterlist);
    while (elem != list_end(&base->iterlist))
    {
        entry = container_of(elem, struct vtsdirtyaddr, iter);
        vtsdirtyset_insert(copy, entry->ptr);
        elem = list_next(elem);
    }

    pthread_mutex_unlock(&base->lock);
    
    return copy;
}

/** Destroys a created dirty address set */
void vtsdirtyset_delete(struct vtsdirtyset *set)
{
    struct list_elem *iter;
    struct vtsdirtyaddr *entry;

    pthread_mutex_lock(&set->lock);

    while (!list_empty(&set->iterlist))
    {
        iter = list_pop_front(&set->iterlist);
        entry = list_entry(iter, struct vtsdirtyaddr, iter);
        __vtsdirtyaddr_delete(entry);
    }

    pthread_mutex_unlock(&set->lock);

    pthread_mutex_destroy(&set->lock);
    mcfree(set);
}

/** Inserts the new address into the set */
void vtsdirtyset_insert(struct vtsdirtyset *set, void *addr)
{
    pthread_mutex_lock(&set->lock);
    __vtsdirtyset_insert(set, addr);
    pthread_mutex_unlock(&set->lock);
}

void *vtsdirtyset_remove(struct vtsdirtyset *set, void *addr)
{
    pthread_mutex_lock(&set->lock);
    addr = __vtsdirtyset_remove(set, addr);
    pthread_mutex_unlock(&set->lock);

    return addr;
}

void *vtsdirtyset_remove_any(struct vtsdirtyset *set)
{
    void *addr;

    pthread_mutex_lock(&set->lock);
    addr = __vtsdirtyset_remove_any(set);
    pthread_mutex_unlock(&set->lock);

    return addr;
}