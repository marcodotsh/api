#ifndef H_SORTING_ALGO
#define H_SORTING_ALGO

#include <stdlib.h>
#include <limits.h>
#include "common.h"

/*
 * Bubble Sort is very simple, but not very time efficient:
 * - Time complexity: $O(n^2)$
 * - Space complexity: $O(1)$
 * - Stability: Yes if we remember to swap only when > or < and not >= or <=
 * - Optimizations: Flag to check if it is already sorted and return early
 */ 
void bubbleSort(int * a, int n) {
    int i, j, tmp, flag=0;

    for(i=0;i<n&&!flag;i++){
        flag = 1;
        for(j=0;j<n-1-i;j++){
            if(a[j]>a[j+1]){
                flag = 0;
                tmp = a[j];
                a[j] = a[j+1];
                a[j+1] = tmp;
            }
        }
    }

    return;
}

/*
 * Insertion sort is simple, and has an advantage when the array is almost completely ordered:
 * - Time complexity: Best case: $\Theta(n)$ Worst case: $\Theta(n^2)$ in general 
 * - Space complexity: $O(1)$
 * - Stability: Yes if we remember to slide only when > or < and not >= or <=
 * - Optimizations:
 */ 

void insertionSort(int * a, int n) {
    int i, tmp, j;
    for(i=1;i<n;i++) {
        tmp = a[i];
        j=i-1;
        while(j>=0 && a[j]>tmp) {
            a[j+1] = a[j];
            j--;
        }
        a[j+1] = tmp;
    }

    return;
}

/*
 * Merge sort is a recursive divide et impera algorithm
 * it is time efficient with a drawback in space complexity:
 * - Time complexity: $\Theta(n\log(n))$ 
 * - Space complexity: $O(n)$
 * - Stability: Yes if we remember to slide only when > or < and not >= or <=
 * - Optimizations: with long arrays we fill stack memory,
 *   maybe using dynamic allocation in heap we can do more
 */ 

void mergeSort(int * a, int n) {
    if(n>=2) {
        int dim1 = n/2;
        int dim2 = n - dim1;

        mergeSort(a,dim1);
        mergeSort(&a[dim1],dim2);

        int * part1;
        part1 = (int *)malloc(sizeof(int)*(dim1+1));
        int * part2;
        part2 = (int *)malloc(sizeof(int)*(dim2+1));
        copyArray(a,part1,dim1);
        part1[dim1] = INT_MAX;
        copyArray(&a[dim1],part2,dim2);
        part2[dim2] = INT_MAX;
        
        int i = 0,j = 0;
        while(i+j<n) {
            if(part1[i]<=part2[j]) {
                a[i+j] = part1[i];
                i++;
            }
            else {
                a[i+j] = part2[j];
                j++;
            }
        }
        free(part1);
        free(part2);
    }
    
    return;
}

int partition(int * a, int n) {

    int pivot = a[0];

    int i = -1;
    int j = n;
    int tmp;

    while(1) {

        do{
            i++;
        }
        while(a[i]<pivot);

        do {
            j--;
        }while(a[j]>pivot);

        if(i>=j) return j;

        tmp = a[i];
        a[i] = a[j];
        a[j] = tmp;

    }

}

void quickSort(int * a, int n) {

    if(n==1) return;
    
    int pivot;

    pivot = partition(a,n);

    quickSort(a,pivot+1);
    quickSort(&(a[pivot+1]),n-pivot-1);
    
    return;
}

void heapSort(int * a, int n) {
    return;
}

void countingSort(int * a, int n, int domain_size) {
    
    int i, idx;
    int * frequency;
    frequency = malloc(domain_size*sizeof(int));

    for(i=0;i<domain_size;i++) frequency[i] = 0;

    for(i=0;i<n;i++) {
        frequency[a[i]]++;
    }

    idx=0;
    for(i=0;i<domain_size;i++) {
        while(frequency[i]>0) {
            a[idx] = i;
            frequency[i]--;
            idx++;
        }
    }
    
    return;
}

void countingStableSort(int * a, int n, int domain_size) {
    
    int sum, i, idx;
    int * frequency, * cumulative, * b;
    frequency = malloc(domain_size*sizeof(int));
    cumulative = malloc(domain_size*sizeof(int));
    b = malloc(n*sizeof(int));

    for(i=0;i<domain_size;i++) frequency[i] = 0;

    for(i=0;i<n;i++) {
        frequency[a[i]]++;
    }

    sum=0;

    for(i=0;i<domain_size;i++) {
        sum += frequency[i];
        cumulative[i] = sum;
    }

    copyArray(a,b,n);

    for(i=n-1;i>=0;i--) {
        idx = cumulative[b[i]] - 1;
        a[idx] = b[i];
        cumulative[b[i]]--;
    }
    
    return;
}



#endif

