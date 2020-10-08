#ifndef __NVSTORE_H__
#define __NVSTORE_H__

#include "list.h"

#include <stddef.h>

void nvstore_init(const char *filename);
void nvstore_shutdown();

void *nvstore_allocpage(size_t npages);
void nvstore_checkpoint();

#endif
