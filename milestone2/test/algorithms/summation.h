#ifndef __SUMMATION_H__
#define __SUMMATION_H__

#include <stdint.h>
#include <stddef.h>

struct summation_args
{
    intptr_t *arr;
    intptr_t answer;
    size_t len;
};

void *summation_tf_checkpointed(void *arg);
intptr_t summation_checkpointed(struct summation_args *args);
intptr_t summation_serial(intptr_t *arr, size_t len);

#endif