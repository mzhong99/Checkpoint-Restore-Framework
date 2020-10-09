#ifndef __NVSTORE_H__
#define __NVSTORE_H__

#include <stddef.h>

#define E_NVFS          40
#define E_UFFDOPEN      41
#define E_KSWOPEN       42
#define E_MMAP          43
#define E_IOCTL         44
#define E_PTHREAD       45
#define E_WRITE         46
#define E_CLOSE         47

int nvstore_init(const char *filename);
int nvstore_shutdown();

void *nvstore_allocpage(size_t npages);
void nvstore_checkpoint();

#endif
