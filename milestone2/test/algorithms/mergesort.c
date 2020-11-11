#include "mergesort.h"
#include "crthread.h"
#include "crheap.h"

#include <unistd.h>
#include <stdio.h>

#include <assert.h>

#define MOVEMENT_UP         "A"
#define MOVEMENT_DOWN       "B"
#define MOVEMENT_RIGHT      "C"
#define MOVEMENT_LEFT       "D"

static void move_cursor(int ntimes, const char *direction)
{
    while (ntimes --> 0)
        printf("\033[1%s", direction);
}

static void show_mergesort_array(int *arr, int low, int high)
{
    int i;

    printf("Merge Step:\n\n\n");
    fflush(stdout);

    move_cursor(2, MOVEMENT_UP);

    printf("+");
    move_cursor(1, MOVEMENT_DOWN);
    move_cursor(1, MOVEMENT_LEFT);
    printf("|");
    move_cursor(1, MOVEMENT_DOWN);
    move_cursor(1, MOVEMENT_LEFT);
    printf("+");
    move_cursor(2, MOVEMENT_UP);

    for (i = 0; i < MERGESORT_LENGTH; i++)
    {
        printf("-");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf(" ");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("-");
        move_cursor(2, MOVEMENT_UP);
        
        printf("--");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(2, MOVEMENT_LEFT);

        printf("%02d", arr[i]);
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(2, MOVEMENT_LEFT);
        printf("--");
        move_cursor(2, MOVEMENT_UP);

        printf("-");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf(" ");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("-");
        move_cursor(2, MOVEMENT_UP);

        printf("+");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("|");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("+");
        move_cursor(2, MOVEMENT_UP);

        fflush(stdout);

        usleep(20000);
    }

    move_cursor(3, MOVEMENT_DOWN);
    printf("\n");

    for (i = 0; i <= MERGESORT_LENGTH * 5; i++)
        printf((i == low * 5 || i == (high + 1) * 5) ? "|" : " ");
    printf("\n\n");

    fflush(stdout);
}

static void show_subarrays(int *leftarr, int leftlen, 
                           int *rightarr, int rightlen)
{
    int i;

    printf("          \n");
    printf("    Left: \n");
    printf("          ");

    move_cursor(2, MOVEMENT_UP);
    printf("+");
    move_cursor(1, MOVEMENT_DOWN);
    move_cursor(1, MOVEMENT_LEFT);
    printf("|");
    move_cursor(1, MOVEMENT_DOWN);
    move_cursor(1, MOVEMENT_LEFT);
    printf("+");
    move_cursor(2, MOVEMENT_UP);

    for (i = 0; i < leftlen; i++)
    {
        printf("-");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf(" ");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("-");
        move_cursor(2, MOVEMENT_UP);
        
        printf("--");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(2, MOVEMENT_LEFT);

        printf("%02d", leftarr[i]);
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(2, MOVEMENT_LEFT);
        printf("--");
        move_cursor(2, MOVEMENT_UP);

        printf("-");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf(" ");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("-");
        move_cursor(2, MOVEMENT_UP);

        printf("+");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("|");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("+");
        move_cursor(2, MOVEMENT_UP);

        fflush(stdout);

        usleep(80000);
    }

    fflush(stdout);
    usleep(1000000);

    printf("           ");
    move_cursor(1, MOVEMENT_DOWN);
    move_cursor(11, MOVEMENT_LEFT);
    printf("    Right: ");
    move_cursor(1, MOVEMENT_DOWN);
    move_cursor(11, MOVEMENT_LEFT);
    printf("           ");

    move_cursor(2, MOVEMENT_UP);
    printf("+");
    move_cursor(1, MOVEMENT_DOWN);
    move_cursor(1, MOVEMENT_LEFT);
    printf("|");
    move_cursor(1, MOVEMENT_DOWN);
    move_cursor(1, MOVEMENT_LEFT);
    printf("+");
    move_cursor(2, MOVEMENT_UP);

    for (i = 0; i < rightlen; i++)
    {
        printf("-");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf(" ");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("-");
        move_cursor(2, MOVEMENT_UP);
        
        printf("--");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(2, MOVEMENT_LEFT);

        printf("%02d", rightarr[i]);
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(2, MOVEMENT_LEFT);
        printf("--");
        move_cursor(2, MOVEMENT_UP);

        printf("-");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf(" ");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("-");
        move_cursor(2, MOVEMENT_UP);

        printf("+");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("|");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("+");
        move_cursor(2, MOVEMENT_UP);

        fflush(stdout);

        usleep(80000);
    }

    usleep(1000000);

    printf("\n\n\n\n");
}

static void show_merge(int *arr, int mid, int nmerge)
{
    int i;

    printf("            \n");
    printf("    Merger: \n");
    printf("            ");

    fflush(stdout);
    usleep(1000000);

    move_cursor(2, MOVEMENT_UP);
    printf("+");
    move_cursor(1, MOVEMENT_DOWN);
    move_cursor(1, MOVEMENT_LEFT);
    printf("|");
    move_cursor(1, MOVEMENT_DOWN);
    move_cursor(1, MOVEMENT_LEFT);
    printf("+");
    move_cursor(2, MOVEMENT_UP);

    for (i = 0; i < nmerge; i++)
    {
        printf("-");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf(" ");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("-");
        move_cursor(2, MOVEMENT_UP);
        
        printf("--");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(2, MOVEMENT_LEFT);

        printf("%02d", arr[mid + i]);
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(2, MOVEMENT_LEFT);
        printf("--");
        move_cursor(2, MOVEMENT_UP);

        printf("-");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf(" ");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("-");
        move_cursor(2, MOVEMENT_UP);

        printf("+");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("|");
        move_cursor(1, MOVEMENT_DOWN);
        move_cursor(1, MOVEMENT_LEFT);
        printf("+");
        move_cursor(2, MOVEMENT_UP);

        fflush(stdout);

        usleep(80000);
    }

    printf("\n\n\n\n");
    fflush(stdout);
    // usleep(1000000);
}

void merge(int *arr, int low, int mid, int high)
{
    int left_len, right_len;
    int left_it, right_it, merge_it;

    int *leftcpy, *rightcpy;

    left_len = mid - low + 1;
    right_len = high - mid;

    leftcpy = crmalloc(left_len * sizeof(*leftcpy));
    rightcpy = crmalloc(right_len * sizeof(*rightcpy));

    for (left_it = 0; left_it < left_len; left_it++)
        leftcpy[left_it] = arr[low + left_it];
    
    for (right_it = 0; right_it < right_len; right_it++)
        rightcpy[right_it] = arr[mid + 1 + right_it];

    left_it = right_it = 0;
    merge_it = low;

    while (left_it < left_len && right_it < right_len)
    {
        if (leftcpy[left_it] <= rightcpy[right_it])
            arr[merge_it++] = leftcpy[left_it++];
        else
            arr[merge_it++] = rightcpy[right_it++];
    }

    while (left_it < left_len)
        arr[merge_it++] = leftcpy[left_it++];
    
    while (right_it < right_len)
        arr[merge_it++] = rightcpy[right_it++];

    show_subarrays(leftcpy, left_len, rightcpy, right_len);

    crfree(leftcpy);
    crfree(rightcpy);
}

void __mergesort(int *arr, int low, int high)
{
    int mid;

    if (low >= high)
        return;

    
    mid = low + ((high - low) / 2);

    __mergesort(arr, low, mid);
    __mergesort(arr, mid + 1, high);

    show_mergesort_array(arr, low, high);
    usleep(1000000);

    merge(arr, low, mid, high);
    show_merge(arr, mid, high - low + 1);

    crthread_checkpoint();
}

void mergesort(int *arr, int len)
{
    crthread_checkpoint();
    __mergesort(arr, 0, len - 1);
    show_mergesort_array(arr, 0, len - 1);
    usleep(1000000);
}