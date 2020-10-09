#include "nvstore.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

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

#include "nvaddrlist.h"
#include "nvblock.h"
#include "nvaddrtable.h"

/******************************************************************************/
/** Macros, Definitions, and Static Variables: nvstore ---------------------- */
/******************************************************************************/

/* non-volatile storage manager struct */
struct nvstore
{
    struct list blocks;         /* master container of allocated pages */
    struct nvaddrlist *dirty;   /* list of dirty pages to checkpoint */
    struct nvaddrtable *table;  /* used to determine more data about a page */

    pthread_t uffdworker;       /* thread for handling user page faults */
    int uffd;                   /* userfaultfd message file descriptor */
    int killfd;                 /* killswitch for userfaultfd handler */

    void *tmppage;              /* the mmapped page to load upon pagefault */

    FILE* nvfs;                 /* file where the heap pages are stored */
    off_t filesize;             /* size of current file in bytes */
};

/** static variables for holding nvstore state (use like it's an object) */
static struct nvstore s_nvstore;
static struct nvstore *const self = &s_nvstore;

/** task function - do not call directly, run on separate thread */
static void *nvstore_tf_uffdworker(__attribute__((unused))void *args);

/******************************************************************************/
/** Private Implementation: nvstore ----------------------------------------- */
/******************************************************************************/
static void nvstore_handle_pagefault()
{
    struct uffdio_copy uffdio_copy;
    struct uffd_msg msg;
    void *addr, *pgstart;

    int nread;

    nread = read(self->uffd, &msg, sizeof(msg));
    assert(nread != -1);
    assert(msg.event == UFFD_EVENT_PAGEFAULT);

    addr = (void *)msg.arg.pagefault.address;
    pgstart = (void *)((uintptr_t)addr & ~(sysconf(_SC_PAGE_SIZE) - 1));

    nvaddrlist_insert(self->dirty, addr);

    uffdio_copy.src = (uintptr_t)self->tmppage;
    uffdio_copy.dst = (uintptr_t)pgstart;

    uffdio_copy.len = sysconf(_SC_PAGE_SIZE);
    uffdio_copy.mode = 0;
    uffdio_copy.copy = 0;

    assert(ioctl(self->uffd, UFFDIO_COPY, &uffdio_copy) != -1);
}

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

/******************************************************************************/
/** Public-Facing API ------------------------------------------------------- */
/******************************************************************************/
int nvstore_init(const char *filename)
{
    struct uffdio_api api;
    struct stat st;
    int rc;

    /* initialization of core data structures */
    list_init(&self->blocks);
    self->dirty = nvaddrlist_new(NVADDRLIST_INIT_POWER);
    self->table = nvaddrtable_new(NVADDRTABLE_INIT_POWER);

    /* initialization of a non-volatile file */
    self->nvfs = fopen(filename, "w+");
    if (self->nvfs == NULL)
        return E_NVFS;

    if (stat(filename, &st) != 0)
        return E_NVFS;

    self->filesize = st.st_size;

    /* initialization of the empty page to be loaded in */
    self->tmppage = mmap(NULL, sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (self->tmppage == MAP_FAILED)
        return E_MMAP;

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

void *nvstore_allocpage(size_t npages)
{
    struct uffdio_register reg;
    struct nvblock *block;
    
    block = nvblock_new(NULL, npages, 0);
    list_push_back(&self->blocks, &block->elem);
    nvaddrtable_insert(self->table, block);

    reg.range.start = (uintptr_t)block->pgstart;
    reg.range.len = block->npages * sysconf(_SC_PAGE_SIZE);
    reg.mode = UFFDIO_REGISTER_MODE_MISSING;

    assert(ioctl(self->uffd, UFFDIO_REGISTER, &reg) != -1);

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
    // size_t i;
    // void *page;

    // for (i = 0; i < self->dirty->len; i++)
    // {
    //     page = self->dirty->addrs[i];
    // }

    nvaddrlist_clear(self->dirty);
}
