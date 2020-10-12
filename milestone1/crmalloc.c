#include <unistd.h>
#include "crmalloc.h"

mm_instance() {
  static struct memory_manager mm;
  static bool constructed = false;

  if (!constructed) {
    const size_t PAGESIZE = (size_t) sysconf(_SC_PAGESIZE);
    list_init(&mm.pageSets);
    list_init(&mm.freeBlocks);
  }

  return &mm;
}

struct block *make_block(size_t bytes) {

  struct memory_manager* mm = mm_instance();
  struct pageSet *ps;

  // Determine number of pages to request
  size_t blockSize = bytes // CHANGE THIS TO ACCOUNT FOR ALIGNMENT + METADATA
  ps->nPages = blockSize/PAGESIZE + (blockSize % PAGESIZE != 0);

  // Request memory
  ps->start = nvstore_allocpage(ps->nPages);

  // Verify request was successful
  if (!(ps->start)) { return NULL; }
  list_push_back(&(mm->pageSets), &(ps->elem));

  // Split ps into blocks
  list_init(&(ps->blocks));

  // Make block for section of ps that will be in use
  struct block *b;
  b->pageSet = ps;
  b->offset = 0;
  b->size = blockSize;
  b->payload = ps->start;
  b->inuse = true;
  list_push_back(&(ps->blocks), &(b->elem_ps));

  // Set aside unused section of ps as a free block
  struct block *freeBlock;
  freeBlock->pageSet = ps;
  freeBlock->offset = blockSize;
  freeBlock->size = ps->nPages * PAGESIZE - blockSize;
  freeBlock->payload = (void*) (((char*) ps->start) + freeBlock->offset);
  freeBlock->inuse = false;
  list_push_back(&(ps->blocks), &(freeBlock->elem_ps));
  list_push_back(&(mm->freeBlocks), &(freeBlock->elem_fb));

  // Return usable block
  return b;
}

void *cr_malloc(size_t bytes) {

  // Ignore spurious requests
  if (bytes == 0) { return NULL; }

  // Search for free blocks

  // No free blocks are available; allocate new block
  struct block *b = make_block(bytes);
  return b->payload;
}
