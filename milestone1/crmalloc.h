#include <stdbool.h>
#include "list.h"

extern const size_t PAGESIZE;

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

void *cr_malloc(size_t);
void cr_free(void *);
void *cr_realloc(void *, size_t);