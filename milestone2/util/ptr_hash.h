#ifndef __PTR_HASH_H__
#define __PTR_HASH_H__

#include <stdint.h>

static inline uint64_t ptr_hash(void *ptr) 
{
    uint64_t x = (uint64_t)ptr;

    x = (x ^ (x >> 30)) * (uint64_t)(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * (uint64_t)(0x94d049bb133111eb);
    x = (x ^ (x >> 31));

    return x;
}

#endif