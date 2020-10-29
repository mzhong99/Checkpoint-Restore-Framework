#ifndef __NVBLOCK_H__
#define __NVBLOCK_H__

#include "vtslist.h"

#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>

/**
 * Non-volatile blocks of data, allocated through mmap. Despite its name having
 * the subtitle "non-volatile", the actual behavior of this block still needs 
 * some form of reconstruction after a heap allocation.  
 * 
 * When using [vblock], it's probably better to think of it as a data structure
 * which MANAGES non-volatile data. Specifically, the data pointed to by the 
 * member [pgstart] IS a pointer to non-volatile memory, but everything else 
 * is NOT. After a reset, it is the responsibility of nvstore to recreate its
 * indexes of [vblock] structs by respecifying its file offset and starting
 * address using [vblock_new()].
 * 
 * When calling [vblock_new(void *pgaddr, size_t npages, off_t offset)], if the
 * pgaddr supplied is NULL, the function will create a new anonymous [mmap()]
 * space for you. On the other hand, if the pgaddr is NOT null, then the 
 * [mmap()] call will use the provided address to create its mapped region.
 * The purpose of this is so that addresses which were allocate upon previous
 * initializations of this non-volatile memory system now point to the same 
 * valid memory as before, which means that a user program can continue
 * execution with the assumption that its memory space never even changed.
 * 
 * It is the caller's responsibility, however, to specify where
 * exactly in the file this [mmap()] region is to be stored through the [offset]
 * parameter. It is also the caller's responsibility to actually register any
 * memory management handlers through [ioctl()] on the segment provided by the
 * [pgstart] member variable. Each of these operations needs to be performed 
 * upon ANY heap shutdown and restart.
 */
struct vblock
{
    /* basic housekeeping data                                                */
    /* ---------------------------------------------------------------------- */
    struct vtslist_elem tselem; /* so that we can insert into a vtslist       */
    size_t npages;              /* number of pages allocated in this block    */
    void *pgstart;              /* the actual start of the page               */

    /* offset data                                                            */
    /* ---------------------------------------------------------------------- */
    off_t offset;               /* offset in file where block data is stored  */
    off_t offset_pgstart;       /* offset in file where page data is stored   */
};

/* constructor and destructor functions for a non-volatile block */
struct vblock *vblock_new(void *pgaddr, size_t npages, off_t offset);
void vblock_delete(struct vblock *block);

off_t vblock_nvfsize(struct vblock *block);
off_t vblock_pgoffset(struct vblock *block, void *addr);

void vblock_dumptofile(struct vblock *block, FILE *file);
void vblock_dumpbypage(struct vblock *block, FILE *file, void *addr);

#endif
