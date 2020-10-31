#ifndef __VTHREADTABLE_H__
#define __VTHREADTABLE_H__

#include "memcheck.h"
#include "crthread.h"

#include <sys/types.h>
#include <stddef.h>

/**
 * Implementation of a volatile, thread-safe thread table. This data structure
 * allows our system to retrieve a crthread handle from using only the 
 * [pthread_self()] function. 
 * 
 * This data structure is volatile, but thread safe. What this means is that,
 * after the program is restarted from a crash, each thread needs to be 
 * reinserted into this table during the restoration process. 
 * 
 * This design is intentional - since we have no guarantee that pthread_t values
 * will be granted consistently among multiple executions of the same program, 
 * we have to manually reinsert the thread handles ourselves.
 * 
 * Since the thread table manages all crthread handles in the context of this
 * program, we need only a single table. Thus, instead of the traditional 
 * [*_new()] and [*_delete()] functions, we only provide an [*_init()] and
 * [*_cleanup()] instead, both of which operate on a single global instance of
 * this table.
 * 
 * Each crthread should only ever attempt to perform the following actions:
 *  1.) Insert itself into this table.
 *  2.) Search for itself from this table.
 *  3.) Remove itself from this table.
 * 
 * In other words, while multiple threads CAN impart themselves into this table,
 * no one thread should ever attempt to perform operations on ANOTHER THREAD 
 * with respect to this table. The thread safety is only designed for the three
 * purposes as described above.
 */

/** Initializes the global thread table */
void vtsthreadtable_init();

/** Cleans up the global thread table, deleting all contained threads. */
void vtsthreadtable_cleanup();

/** Inserts a created thread handle into this table. */
void vtsthreadtable_insert(struct crthread *handle);

/** 
 * Finds the crthread handle corresponding to the pthread_t presented. Do not
 * perform a find, followed by a remove. Use only one of the two functions at
 * a time.
 */
struct crthread *vtsthreadtable_find(pthread_t id);

/** 
 * Removes the crthread handle corresponding to the pthread_t presented. Do not
 * perform a find, followed by a remove. Use only one of the two functions at
 * a time.
 */
struct crthread *vtsthreadtable_remove(pthread_t id);

#endif