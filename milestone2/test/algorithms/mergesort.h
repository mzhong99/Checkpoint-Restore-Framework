#ifndef __MERGESORT_H__
#define __MERGESORT_H__

#define MERGESORT_LENGTH    16

void __mergesort(int *arr, int low, int high);
void mergesort(int *arr, int len);
void merge(int *arr, int low, int mid, int high);

#endif