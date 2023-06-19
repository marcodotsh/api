#define _GNU_SOURCE
#define PRINT_THRESHOLD 10

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/resource.h>

#include "common.h"
#include "algo.h"

struct testAlgoArg_t {
    char * algoName;
    int * array;
    int arrayDim;
};

pthread_mutex_t print;

void printOrder(int * a, int n) {
    if(checkOrder(a,n))
        printf("The array is in order.\n");
    else
        printf("The array is not in order.\n");

    return;
}

void * testAlgo(void * arg) {

    struct rusage threadRes;

    char * algoName = ((struct testAlgoArg_t *)arg)->algoName;
    int * a = ((struct testAlgoArg_t *)arg)->array;
    int n = ((struct testAlgoArg_t *)arg)->arrayDim;

    int * testArray;

    testArray = malloc(sizeof(int)*n);

    copyArray(a,testArray,n);

    if(strcmp(algoName,"bubble") == 0) {
        printf("Testing Bubble Sort\n");
        bubbleSort(testArray,n);
    }
    else if(strcmp(algoName,"insert") == 0) {
        printf("Testing Insertion Sort\n");
        insertionSort(testArray,n);
    }
    else if(strcmp(algoName,"merge") == 0) {
        printf("Testing Merge Sort\n");
        mergeSort(testArray,n);
    }
    else if(strcmp(algoName,"quick") == 0) {
        printf("Testing Quick Sort\n");
        quickSort(testArray,n);
    }
    else if(strcmp(algoName,"quick-glibc") == 0) {
        printf("Testing Quick Sort Glibc\n");
        qsort(testArray,n,sizeof(int),cmpintasc);
    }
    else if(strcmp(algoName,"count") == 0) {
        if(MAX_RAND>8192) printf("Domain is too big to use counting sort.\n");
        else {
            printf("Testing Counting Sort\n");
            countingSort(testArray,n, MAX_RAND);
        }
    }
    else if(strcmp(algoName,"count-stable") == 0) {
        if(MAX_RAND>8192) printf("Domain is too big to use counting sort.\n");
        else {
            printf("Testing Counting Stable Sort\n");
            countingStableSort(testArray,n, MAX_RAND);
        }
    }
    else {
        printf("Error: there is no algorithm with such name\n");
        return NULL;
    }

    getrusage(RUSAGE_THREAD, &threadRes);

    pthread_mutex_lock(&print);
    if(n<=PRINT_THRESHOLD){
        printf("[%s]\n", algoName);
        printArray(testArray,n);   
    }
    
    printf("[%s] ", algoName);
    printOrder(testArray,n);
    printResources(&threadRes);
    pthread_mutex_unlock(&print);

    free(testArray);

    return NULL;
}

int main(int argc, char * argv[]) {

    if(argc==1) {
        printf("Please select at least one algorithm:\n"
                "bubble\n"
                "insert\n"
                "merge\n"
                "quick\n"
                "quick-glibc\n"
                "count\n"
                "count-stable\n"
                "\n");
        return 0;
    }

    int size;

    printf("Choose size of array: ");
    scanf("%d", &size);

    int * a;

    a = malloc(sizeof(int)*size);

    initializeArray(a,size);
    
    if(size<=PRINT_THRESHOLD){
        printArray(a,size);  
    }
    
    pthread_mutex_init(&print, NULL);

    int nthreads = argc - 1;

    pthread_t tID[nthreads];

    struct testAlgoArg_t arg[nthreads];

    
    for(int i=0;i<nthreads;i++) {
        arg[i].algoName = argv[i+1];
        arg[i].array = a;
        arg[i].arrayDim = size;
        pthread_create(&tID[i], NULL, &testAlgo, &arg[i]);
    }

    for(int i=0;i<nthreads;i++) {
        pthread_join(tID[i],NULL);
    }

    free(a);

    return 0;

}
