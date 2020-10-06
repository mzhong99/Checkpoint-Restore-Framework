#ifndef __NVSTORE_H__
#define __NVSTORE_H__

#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

struct nvchunk
{
    struct list_elem elem;
    size_t npages;
    void *pgstart;
    void *pgend;
};

struct nvchunk *nvchunk_new(size_t npages);
void nvchunk_delete(struct nvchunk *nvchunk);

int nvstore_init(const char *filename);
int nvstore_shutdown();

void *nvstore_allocpage(size_t npages);
int nvstore_checkpoint();

#endif
