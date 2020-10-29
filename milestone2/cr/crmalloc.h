#ifndef __CRMALLOC_H__
#define __CRMALLOC_H__

#include <stdbool.h>
#include <unistd.h>
#include "list.h"

struct memory_manager {
    struct list free;
};

union boundary_tag {
    struct {
        size_t payload_size; // does not include metadata size
        bool inuse;
    } data;
};

struct block {
    void *mem_lo;
    void *mem_hi;
    union boundary_tag tag; // left tag
    struct list_elem elem;
    char payload[0];
};

/* Left boundary tag, list_elem, right boundary tag */
#define METADATA_SIZE (sizeof(struct block) + (sizeof(union boundary_tag)))
#define PAGESIZE (sysconf(_SC_PAGESIZE))

void mm_init(struct memory_manager *mm);

void *crmalloc(size_t);
void crfree(void *);
void *crrealloc(void *, size_t);

#endif