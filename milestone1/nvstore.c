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

    int nvfd;                   /* file where the heap pages are stored */
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
void nvstore_init(const char *filename)
{
    struct uffdio_api api;
    struct stat st;

    list_init(&self->blocks);
    self->dirty = nvaddrlist_new(NVADDRLIST_INIT_POWER);

    self->nvfd = open(filename, O_RDWR | O_CREAT);

    stat(filename, &st);
    self->filesize = st.st_size;

    self->tmppage = mmap(NULL, sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    self->killfd = eventfd(0, EFD_SEMAPHORE);
    self->uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);

    api.api = UFFD_API;
    api.features = 0;
    ioctl(self->uffd, UFFDIO_API, &api);

    pthread_create(&self->uffdworker, NULL, nvstore_tf_uffdworker, NULL);
}

void *nvstore_allocpage(size_t npages)
{
    struct nvblock *block;
    
    block = nvblock_new(NULL, npages);
    list_push_back(&self->blocks, &block->elem);

    return block->pgstart;
}

void nvstore_shutdown()
{
    struct list_elem *elem;
    struct nvblock *block;
    uint64_t postkill = 1;

    write(self->killfd, &postkill, sizeof(postkill));
    pthread_join(self->uffdworker, NULL);

    nvaddrlist_delete(self->dirty);

    while (!list_empty(&self->blocks))
    {
        elem = list_pop_back(&self->blocks);
        block = list_entry(elem, struct nvblock, elem);

        nvblock_delete(block);
    }

    close(self->nvfd);
    close(self->uffd);
    close(self->killfd);

    munmap(self->tmppage, sysconf(_SC_PAGE_SIZE));
}

void nvstore_checkpoint()
{
    size_t i;
    void *page;

    for (i = 0; i < self->dirty->len; i++)
    {
        page = self->dirty->addrs[i];
    }

    nvaddrlist_clear(self->dirty);
}
