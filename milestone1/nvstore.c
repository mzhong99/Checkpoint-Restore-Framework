#include "nvstore.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#include <syscall.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <linux/userfaultfd.h>

#include <pthread.h>
#include <assert.h>
#include <alloca.h>

#include "list.h"

#include "nvaddrlist.h"
#include "nvblock.h"
#include "nvaddrtable.h"

/******************************************************************************/
/** Macros, Definitions, and Static Variables: nvstore ---------------------- */
/******************************************************************************/

/* metadata for non-volatile storage - each element is saved to file */
struct nvmetadata
{
    uint32_t writelock;     /* set to 0 when writing and reset to 1 when done */
    struct list sysres;     /* list of system resources                       */
};

/* used to manually control the write lock for the metadata */
static void nvmetadata_lock(struct nvmetadata *meta);
static void nvmetadata_unlock(struct nvmetadata *meta);

/* non-volatile storage manager struct */
struct nvstore
{
    /* bookkeeping data strctures                                             */
    /* ---------------------------------------------------------------------- */
    struct list blocks;         /* master container of allocated pages        */
    struct nvaddrlist *dirty;   /* list of dirty pages to checkpoint          */
    struct nvaddrtable *table;  /* used to relocate block from raw page addr  */

    /* userfaultfd handler and file descriptors                               */
    /* ---------------------------------------------------------------------- */
    pthread_t uffdworker;       /* thread for handling user page faults       */
    int uffd;                   /* userfaultfd message file descriptor        */
    int killfd;                 /* killswitch for userfaultfd handler         */
    void *tmppage;              /* the mmapped page to load upon pagefault    */

    /* non-volatile filesystem used to store data on checkpoint               */
    /* ---------------------------------------------------------------------- */
    FILE* nvfs;                 /* file where the heap pages are stored       */
    off_t filesize;             /* size of file, tracked manually             */
    struct nvmetadata *meta;    /* metadata object                            */
};

/** static variables for holding nvstore state (use like it's an object) */
static struct nvstore s_nvstore;
static struct nvstore *const self = &s_nvstore;

/** task function - do not call directly, run on separate thread */
static void *nvstore_tf_uffdworker(__attribute__((unused))void *args);

/** helper init functions */
static int nvstore_initnvfs(const char *filename);
static int nvstore_initmmap();
static int nvstore_inituffdworker();
static void nvstore_initmeta();

/** retrieves and allocates the next block from file, with bookkeeping */
static struct nvblock *nvstore_fetchnvfs();

/** helper allocation function, allows address specification */
static struct nvblock *__nvstore_allocpage(size_t size, void *addr);

/******************************************************************************/
/** Private Implementation: nvmetadata -------------------------------------- */
/******************************************************************************/
static void nvmetadata_lock(struct nvmetadata *meta)
{
    struct nvblock *metablock;

    meta->writelock = 0;
    metablock = nvaddrtable_find(self->table, meta);

    fseek(self->nvfs, metablock->offset_pgstart, SEEK_SET);
    fwrite(&meta->writelock, 1, sizeof(meta->writelock), 
           self->nvfs);
    fflush(self->nvfs);
}

static void nvmetadata_unlock(struct nvmetadata *meta)
{
    struct nvblock *metablock;

    meta->writelock = 1;
    metablock = nvaddrtable_find(self->table, meta);

    fseek(self->nvfs, metablock->offset_pgstart, SEEK_SET);
    fwrite(&meta->writelock, 1, sizeof(meta->writelock), 
           self->nvfs);
    fflush(self->nvfs);
}

/******************************************************************************/
/** Private Implementation: nvstore ----------------------------------------- */
/******************************************************************************/

/**
 * Allocates a block, placing it into our bookkeeping data structures and 
 * registering it to the pagefault handler so that when a pagefault occurs, the
 * handler actually detects the access.
 *
 * @param npages:   the number of pages to allocate - must be greater than zero
 * @param addr:     the mmap address at which to place this memory
 *
 * @return an allocated memory block which is already registered in our internal
 *         data structures. data can be freely read and written to the block's
 *         pagedata and should automatically be checkpointed when we call the
 *         checkpointing function.
 */
static struct nvblock *__nvstore_allocpage(size_t npages, void *addr)
{
    struct uffdio_register reg;
    struct nvblock *block;
    
    /* raw allocation and mmap() - offset should be at end of file */
    block = nvblock_new(addr, npages, self->filesize);

    /* if this allocation was brand-new (no backing address) then allocate space
     * in the file for the new block */
    if (addr == NULL)
        nvblock_dumptofile(block, self->nvfs);

    /* increment the filesize by the amount of data we just wrote */
    self->filesize += nvblock_nvfsize(block);

    /* insert the block into our bookkeeping data structures */
    list_push_back(&self->blocks, &block->elem);
    nvaddrtable_insert(self->table, block);

    /* finally, set up params to register the block to trigger pagefaults... */
    reg.range.start = (uintptr_t)block->pgstart;
    reg.range.len = block->npages * sysconf(_SC_PAGE_SIZE);
    reg.mode = UFFDIO_REGISTER_MODE_MISSING;

    /* and register the block itself. */
    assert(ioctl(self->uffd, UFFDIO_REGISTER, &reg) != -1);

    /* finally, ensure that pagefaults WILL happen upon access */
    madvise(block->pgstart, block->npages * sysconf(_SC_PAGE_SIZE), 
            MADV_DONTNEED);

    return block;
}

/**
 * When a pagefault occurs, this function handles the swapping back in of a new
 * page along with logging that the page was touched at some point. The log will
 * be used for checkpointing purposes later.
 */
static void nvstore_handle_pagefault()
{
    struct uffdio_copy uffdio_copy;
    struct uffd_msg msg;
    void *addr, *pgstart;

    int nread;

    /* read in the new pagefault message and ensure we actually pagefaulted */
    nread = read(self->uffd, &msg, sizeof(msg));
    assert(nread != -1);
    assert(msg.event == UFFD_EVENT_PAGEFAULT);

    /* retrieve the offending address and get its original page */
    addr = (void *)msg.arg.pagefault.address;
    pgstart = (void *)((uintptr_t)addr & ~(sysconf(_SC_PAGE_SIZE) - 1));

    /* log the touched page as dirty */
    nvaddrlist_insert(self->dirty, pgstart);

    /* specify the new page to swap back in to finish the pagefault */
    uffdio_copy.src = (uintptr_t)self->tmppage;
    uffdio_copy.dst = (uintptr_t)pgstart;

    uffdio_copy.len = sysconf(_SC_PAGE_SIZE);
    uffdio_copy.mode = 0;
    uffdio_copy.copy = 0;

    /* finally, copy the new page in - this unblocks the offending thread */
    assert(ioctl(self->uffd, UFFDIO_COPY, &uffdio_copy) != -1);
}

/**
 * The worker thread function which services pagefaults. The input argument is
 * not used. Polls for events from two file descriptors - one is the uffd itself
 * and the other is a killfd which signifies when this worker routine is to be
 * killed.
 *
 * Does not return anything meaningful.
 */
static void *nvstore_tf_uffdworker(__attribute__((unused))void *args)
{
    struct pollfd pollfds[2]; /* 0: uffd, 1: killfd */
    int nready;

    pollfds[0].fd = self->uffd;
    pollfds[0].events = POLLIN;

    pollfds[1].fd = self->killfd;
    pollfds[1].events = POLLIN;

    for (;;)
    {
        nready = poll(pollfds, 2, -1);
        assert(nready != -1);

        if ((pollfds[0].revents & POLLIN) != 0)
            nvstore_handle_pagefault();

        if ((pollfds[1].revents & POLLIN) != 0)
            break;
    }

    return NULL;
}

static int nvstore_initnvfs(const char *filename)
{
    /* initialization of container bookkeeping data structures */
    list_init(&self->blocks);
    self->dirty = nvaddrlist_new(NVADDRLIST_INIT_POWER);
    self->table = nvaddrtable_new(NVADDRTABLE_INIT_POWER);

    /* initialization of non-volatile file - DO NOT USE APPEND; instead, try to
     * first open under read mode and if the file doesn't exist, reopen under
     * write mode instead. (fixing this bug was painful good god) 
     *
     * opening the file under append will prevent SEEK_SET from ever moving
     * before the end of the file which is a problem when you want to modify 
     * the file. */
    self->nvfs = fopen(filename, "r+");
    if (self->nvfs == NULL)
        self->nvfs = fopen(filename, "w+");

    if (self->nvfs == NULL)
        return E_NVFS;

    /* appending inits the file offset to EOF - move back to start */
    fseek(self->nvfs, 0, SEEK_SET);

    return 0;
}

static int nvstore_initmmap()
{
    /* initialization of the empty page to be loaded in */
    self->tmppage = mmap(NULL, sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (self->tmppage == MAP_FAILED)
        return E_MMAP;

    return 0;
}

static int nvstore_inituffdworker()
{
    struct uffdio_api api;
    int rc;

    /* initialization of the killswitch for when the fault handler is closed */
    self->killfd = eventfd(0, EFD_SEMAPHORE);

    if (self->killfd == -1)
        return E_KSWOPEN;

    /* initialization of the userfaultfd itself */
    self->uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);

    if (self->uffd == -1)
        return E_UFFDOPEN;

    api.api = UFFD_API;
    api.features = 0;

    if (ioctl(self->uffd, UFFDIO_API, &api) == -1)
        return E_IOCTL;

    /* initialization of the fault handler thread */
    rc = pthread_create(&self->uffdworker, NULL, nvstore_tf_uffdworker, NULL);
    if (rc != 0)
        return E_PTHREAD;

    return 0;
}

/**
 * Initializes the old metadata and fetches all checkpointed blocks from the
 * non-volatile storage.
 */
static void nvstore_initmeta()
{
    struct nvblock *metablock;

    /* first, set the filesize to 0 and attempt to fetch the metadata */
    self->filesize = 0;
    metablock = nvstore_fetchnvfs();

    if (metablock == NULL)
    {
        /* if no metadata was found, create and init a new metadata block */
        metablock = __nvstore_allocpage(1, NULL);
        self->meta = metablock->pgstart;
        list_init(&self->meta->sysres);
    }
    else
    {
        /* continue to fetch new blocks until no new blocks exist in file */
        self->meta = metablock->pgstart;
        while (nvstore_fetchnvfs() != NULL);
    }

    fflush(self->nvfs);
};

/**
 * Fetches a block of memory from the filesystem, returning a constructed,
 * registered block (already added to bookkeeping data structures if parsing 
 * worked), and returning NULL if parsing failed. The internal filesize counter
 * is also updated.
 */
static struct nvblock *nvstore_fetchnvfs()
{
    struct nvblock *block;
    size_t nread, npages;
    void *tmp, *addr;

    fseek(self->nvfs, self->filesize, SEEK_SET);
    nread = fread(&addr, 1, sizeof(addr), self->nvfs);

    if (nread != sizeof(addr) || addr == NULL)
        return NULL;

    fseek(self->nvfs, self->filesize + sizeof(addr), SEEK_SET);
    nread = fread(&npages, 1, sizeof(npages), self->nvfs);

    if (nread != sizeof(npages) || npages == 0)
        return NULL;

    tmp = alloca(npages * sysconf(_SC_PAGE_SIZE));

    fseek(self->nvfs, self->filesize + sizeof(addr) + sizeof(npages), SEEK_SET);
    nread = fread(tmp, 1, npages * sysconf(_SC_PAGE_SIZE), self->nvfs);

    if (nread != npages * sysconf(_SC_PAGE_SIZE))
        return NULL;

    block = __nvstore_allocpage(npages, addr);
    memcpy(block->pgstart, tmp, npages * sysconf(_SC_PAGE_SIZE));
    return block;
}

/******************************************************************************/
/** Public-Facing API ------------------------------------------------------- */
/******************************************************************************/
int nvstore_init(const char *filename)
{
    int rc;

    rc = nvstore_initnvfs(filename);
    if (rc != 0)
        return rc;

    rc = nvstore_initmmap();
    if (rc != 0)
        return rc;

    rc = nvstore_inituffdworker();
    if (rc != 0)
        return rc;

    nvstore_initmeta();
    return 0;
}

void *nvstore_allocpage(size_t npages)
{
    struct nvblock *block;

    block = __nvstore_allocpage(npages, NULL);
    return block->pgstart;
}

int nvstore_shutdown()
{
    struct list_elem *elem;
    struct nvblock *block;
    uint64_t postkill = 1;

    if (write(self->killfd, &postkill, sizeof(postkill)) != sizeof(postkill))
        return E_WRITE;

    pthread_join(self->uffdworker, NULL);

    nvaddrlist_delete(self->dirty);
    nvaddrtable_delete(self->table);

    if (munmap(self->tmppage, sysconf(_SC_PAGE_SIZE)) != 0)
        return E_MMAP;

    while (!list_empty(&self->blocks))
    {
        elem = list_pop_back(&self->blocks);
        block = list_entry(elem, struct nvblock, elem);

        nvblock_delete(block);
    }

    if (fclose(self->nvfs) != 0)
        return E_NVFS;
    if (close(self->uffd) == -1)
        return E_CLOSE;
    if (close(self->killfd) == -1)
        return E_CLOSE;

    return 0;
}

void nvstore_checkpoint()
{
    size_t i;
    void *addr;
    struct nvblock *block;

    nvmetadata_lock(self->meta);

    for (i = 0; i < self->dirty->len; i++)
    {
        addr = self->dirty->addrs[i];
        block = nvaddrtable_find(self->table, addr);

        nvblock_dumpbypage(block, self->nvfs, addr);
    }

    nvmetadata_unlock(self->meta);
    nvaddrlist_clear(self->dirty);
}
