#include "unittest.h"
#include "memcheck.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <locale.h>

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/
#define TABLE_SIZE 4096

enum mc_type { MC_MALLOC, MC_CALLOC, MC_REALLOC, MC_MMAP };
const char *MC_TYPE_STR[] = {"MC_MALLOC", "MC_CALLOC", "MC_REALLOC", "MC_MMAP"};

struct mc_node
{
    struct mc_node *next;
    enum mc_type type;
    void *addr;
    size_t nmemb;
    size_t size;
};

struct mc_table
{
    size_t nelem;
    size_t btotal;
    size_t nalloc;
    size_t nfree;
    struct mc_node *data[TABLE_SIZE];
};

/* self instance for object-oriented access */
static struct mc_table s_table = 
{ .nelem = 0, .btotal = 0, .nalloc = 0, .nfree = 0, .data = {0} };
static struct mc_table *self = &s_table;

/* hash function */
static uint64_t ptr_hash(void *ptr);

/* node management functions */
static struct mc_node *mc_node_new(enum mc_type type, void *addr, 
                                   size_t nmenb, size_t size);
static void mc_node_delete(struct mc_node *node);

/* table management functions */
static struct mc_node **mc_table_find(void *addr);
static bool mc_table_del(void *addr);
static void mc_table_insert(struct mc_node *node);

/******************************************************************************/
/** Private Implementation -------------------------------------------------- */
/******************************************************************************/
static uint64_t ptr_hash(void *ptr) 
{
    uint64_t x = (uint64_t)ptr;

    x = (x ^ (x >> 30)) * (uint64_t)(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * (uint64_t)(0x94d049bb133111eb);
    x = x ^ (x >> 31);

    return x % TABLE_SIZE;
}

static struct mc_node *mc_node_new(enum mc_type type, void *addr, 
                                   size_t nmemb, size_t size)
{
    struct mc_node *node;

    node = malloc(sizeof(*node));

    node->type = type;
    node->addr = addr;
    node->nmemb = nmemb;
    node->size = size;

    return node;
}

static void mc_node_delete(struct mc_node *node)
{
    free(node);
}

static struct mc_node **mc_table_find(void *addr)
{
    uint64_t hash;
    struct mc_node **it_p;

    hash = ptr_hash(addr);

    for (it_p = &self->data[hash]; *it_p != NULL; it_p = &((*it_p)->next))
        if ((*it_p)->addr == addr)
            return it_p;
    
    return it_p;
}

static bool mc_table_del(void *addr)
{
    struct mc_node **it, *find;

    it = mc_table_find(addr);

    if (*it != NULL)
    {
        find = *it;
        *it = find->next;

        self->nfree++;
        self->nelem--;

        mc_node_delete(find);

        return true;
    }

    return false;
}

static void mc_table_insert(struct mc_node *node)
{
    struct mc_node **find;
    uint64_t hash;

    find = mc_table_find(node->addr);

    if (*find != NULL)
        mc_table_del((*find)->addr);

    hash = ptr_hash(node->addr);
    node->next = self->data[hash];
    self->data[hash] = node;

    self->btotal += node->size * node->nmemb;
    self->nelem++;
    self->nalloc++;
}

/******************************************************************************/
/** Public-Facing API ------------------------------------------------------- */
/******************************************************************************/
void *mc_malloc(size_t size)
{
    struct mc_node *node;
    void *ptr;

    if (size == 0)
        return NULL;

    ptr = malloc(size);
    if (ptr == NULL)
        return ptr;

    node = mc_node_new(MC_MALLOC, ptr, 1, size);
    mc_table_insert(node);

    return ptr;
}

void *mc_calloc(size_t nmemb, size_t size)
{
    struct mc_node *node;
    void *ptr;

    if (nmemb == 0 || size == 0)
        return NULL;

    ptr = calloc(nmemb, size);
    if (ptr == NULL)
        return ptr;

    node = mc_node_new(MC_CALLOC, ptr, nmemb, size);
    mc_table_insert(node);

    return ptr;
}

void *mc_realloc(void *ptr, size_t size)
{
    struct mc_node *node;
    void *rptr;

    if (size == 0)
    {
        mc_free(ptr);
        return NULL;
    }

    rptr = realloc(ptr, size);
    if (rptr == NULL)
        return rptr;

    mc_table_del(ptr);
    node = mc_node_new(MC_REALLOC, rptr, 1, size);
    mc_table_insert(node);

    return rptr;
}

void mc_free(void *ptr)
{
    bool worked; 

    if (ptr == NULL)
        return;

    worked = mc_table_del(ptr);

    if (!worked)
    {
        printf("Attempted to mc_free() unregistered address %p\n", ptr);
        abort();
    }

    free(ptr);
}

void *mc_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
    struct mc_node *node;
    void *ptr;

    if (len == 0)
        return NULL;
    
    ptr = mmap(addr, len, prot, flags, fd, off);

    if (ptr == MAP_FAILED)
        return ptr;

    node = mc_node_new(MC_MMAP, ptr, 1, len);
    mc_table_insert(node);
        
    return ptr;
}

int mc_munmap(void *addr, size_t len)
{
    int rc;

    rc = munmap(addr, len);
    mc_table_del(addr);

    return rc;
}

void mc_report()
{
    struct mc_node *chain;
    size_t i;

    setlocale(LC_NUMERIC, "");

    printf(ANSI_COLOR_CYAN);

    printf("================================================================================\n");
    printf("Memory Check Analysis\n");
    printf("================================================================================\n");

    printf(ANSI_COLOR_RESET);

    printf("%'ld un-freed allocations. %s\n", 
            self->nelem, 
            self->nelem == 0 ? ANSI_COLOR_GREEN "No leaks possible!" ANSI_COLOR_RESET 
                             : ANSI_COLOR_RED "Memory was leaked :(" ANSI_COLOR_RESET);

    printf("    Total Usage: %'ld bytes (%'ld allocations, %'ld frees)\n", 
           self->btotal, self->nalloc, self->nfree);

    for (i = 0; i < TABLE_SIZE; i++)
        for (chain = self->data[i]; chain != NULL; chain = chain->next)
            printf("Allocation: %p (%s) [size=%'ld, nmemb=%'ld]\n",
                   chain->addr, MC_TYPE_STR[chain->type], 
                   chain->size, chain->nmemb);

    printf(ANSI_COLOR_RESET);
}
