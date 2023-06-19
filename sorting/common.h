#ifndef H_SORTING_COMMON
#define H_SORTING_COMMON

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define MAX_RAND 8192 //Limit numbers from 0 to MAX_RAND -  - 11


void initializeArray(int * a, int n ) {

    srand(time(NULL));

    for(int i=0; i<n; i++) {
        a[i] = (rand() % MAX_RAND);     
    }

    return;
}

void printArray(int * a, int n) {
    
    for(int i=0;i<n;i++) {
        printf("%d:\t%d\n", i+1, a[i]);
    }

    return;
}

void copyArray(int * src, int * dst, int n) {
    
    for(int i=0;i<n;i++) {
        dst[i] = src[i];
    }

    return;
}

int checkOrder(int * a, int n) {
    
    for(int i=0;i<n-1;i++) {
        if(a[i]>a[i+1]) return 0;
    }

    return 1;

}

void printResources(struct rusage * res) {
    
    printf( "Resource usage:\n"
            "User CPU time used: %ld.%06lds\n"
            "System CPU time used: %ld.%06lds\n"
            "Maximum resident set size: %ldkB\n"
            "Page reclaims: %ld\n"
            "Page faults: %ld\n"
            "Voluntary context switches: %ld\n"
            "Involuntary context switches: %ld\n\n",
            res->ru_utime.tv_sec,
            res->ru_utime.tv_usec,
            res->ru_stime.tv_sec,
            res->ru_stime.tv_usec,
            res->ru_maxrss,
            res->ru_minflt,
            res->ru_majflt,
            res->ru_nvcsw,
            res->ru_nivcsw
            );

    return;
}

//comparison function to make qsort work with integers
int cmpintasc(const void *n1, const void *n2) {

    return (*(int *)n1 - *(int *)n2);
}

#endif

