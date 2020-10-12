#include <stdbool.h>

extern const size_t PAGESIZE;

struct memory_manager {
  struct list pageSets;
  struct list freeBlocks;
}

struct pageSet {
  size_t nPages;
  struct list blocks;
  void *start;
  struct list_elem elem; // element in pageSets
}

struct block {
  size_t size;
  struct pageSet *pageSet;
  void *payload;
  size_t offset;
  bool inuse;
  struct list_elem elem_ps; // element in associated pageSet
  struct list_elem elem_fb; // element in freeBlocks, may be NULL
};
