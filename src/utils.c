//
// Created by Scott Norton on 6/30/17.
//

#include <stdlib.h>
#include <stdbool.h>
#include "utils.h"

const char *const CommandStrings[] = {
    "LZ_LITERAL",
    "LZ_ITERATE",
    "LZ_ALTERNATE",
    "LZ_ZERO",
    "LZ_REPEAT",
    "LZ_FLIP",
    "LZ_REVERSE"
};

unsigned char bflp(unsigned char inbits) {
    unsigned char outbits = 0;
    for (int j = 0; j < 8; j ++) outbits |= ((inbits & (1 << j)) >> j) << (7 - j);
    return outbits;
}

void swap(int *a, int *b) {
    int swbf = *a;
    *a = *b;
    *b = swbf;
}

// Implementation of quicksort with simultaneous argsort.
void sortwargs(int *src, int *argdest, size_t size, bool initialized) {
    int i, j, k, pivot;
    if (!initialized) {
        for (i = 0; i < size; i++) argdest[i] = i;
    }
    if (size == 2)
    {
        if (src[0] > src[1])
        {
            swap(src, src + 1);
            swap(argdest, argdest + 1);
        }
    } else if (size > 2) {
        pivot = random() % size;
        int pivval = src[pivot];
        int pivarg = argdest[pivot];
        j = 0;
        k = 0;
        for (i = 0; i < size; i ++) {
            if (src[i] != pivval) {
                if (src[i] < pivval) {
                    swap(&src[j], &src[i]);
                    swap(&argdest[j], &argdest[i]);
                    j ++;
                } else {
                    swap(&src[size - k], &src[i]);
                    swap(&argdest[size - k], &argdest[i]);
                    k ++;
                }
            }
        }
        src[j] = pivval;
        argdest[j] = pivarg;
        sortwargs(src, argdest, j, true);
        sortwargs(src + j + 1, argdest + j + 1, size - j - 1, true);
    }
}
