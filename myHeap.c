// COMP1521 18s1 Assignment 2
// Implementation of heap management system
// completed by <QWERTYFRIES_>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myHeap.h"

// minimum total space for heap
#define MIN_HEAP  4096
// minimum amount of space for a free Chunk (excludes Header)
#define MIN_CHUNK 32

#define ALLOC     0x55555555
#define FREE      0xAAAAAAAA

typedef unsigned int uint;   // counters, bit-strings, ...

typedef void *Addr;          // addresses

typedef struct {             // headers for Chunks
    uint  status;             // status (ALLOC or FREE)
    uint  size;               // #bytes, including header
} Header;

static Addr  heapMem;        // space allocated for Heap
static int   heapSize;       // number of bytes in heapMem
static Addr *freeList;       // array of pointers to free chunks
static int   freeElems;      // number of elements in freeList[]
static int   nFree;          // number of free chunks
// private function
// round up in multiple of 4
static int roundUp (int x)
{
    int a = x % 4;
    
    if (a != 0) x = (x - a) + 4;
    
    return x;
}
// initialise heap
int initHeap(int size)
{
    // TODO
    if (size < MIN_HEAP) size = MIN_HEAP;
    if ((size % 4) != 0) size = roundUp(size);
    // number of elements in freeList[]
    freeElems = size / MIN_CHUNK;
    
    heapMem = malloc(size);
    freeList = malloc(freeElems * sizeof(int));
    
    if (heapMem != NULL && freeList != NULL)
    {
        memset(heapMem, 0, size);
        memset(freeList, 0, sizeof(int) * freeElems);
        // initialise heapMem a free chunk
        Addr new = heapMem;
        Header * chunk;
        chunk = (Header *) new;
        chunk -> status = FREE;
        chunk -> size = size;
        heapSize = size;
        nFree = 1;
        freeList[0] = heapMem;
        
        return 0;
    }
    // fails to malloc either
    else return -1;
}

// clean heap
void freeHeap()
{
    free(heapMem);
    free(freeList);
}

// allocate a chunk of memory
void *myMalloc(int size)
{
    // TODO
    if (size < 1) return NULL;
    // make sures the size is a multiple of 4
    if ((size % 4) != 0) size = roundUp(size);
    
    int minSize = size + sizeof(Header);
    int reqSize = size + sizeof(Header) + MIN_CHUNK;
    
    Addr new = NULL;
    Header * chunk;
    Addr new2 = NULL;
    
    int i = 0;
    int notFound = 1;
    int found = 0;
    int prevSize = heapSize;
    while (i < nFree)
    {
        chunk = (Header *) freeList[i];
        // find the smallest chunk
        if (chunk -> size >= minSize && chunk -> size <= prevSize)
        {
            notFound = 0;
            found = i;
            prevSize = chunk -> size;
        }
        i ++;
    }
    if (notFound) return NULL;
    // smallest free chunk
    chunk = (Header *) freeList[found];
    // free chunk is equals or smaller than N + HeaderSize + MIN_CHUNK
    if (chunk -> size <= reqSize)
    {
        chunk -> status = ALLOC;
        
        new2 = (Addr)(char *)chunk + sizeof(Header);
        // update freeList[]
        for (int j = 0; j < nFree; j ++)
        {
            freeList[j] = freeList[j + 1];
        }
        nFree --;
    }
    // free chunk is larger than N + HeaderSize + MIN_CHUNK
    else if (chunk -> size > reqSize)
    {
        // split into 2 chunks
        int fsize = chunk -> size;
        // allocated chunk size
        chunk -> size = minSize;
        chunk -> status = ALLOC;
        int asize = chunk -> size;
        
        new2 = (Addr)(char *)chunk + sizeof(Header);
        // upper chunk
        new = (Addr)(char *) freeList[found] + chunk -> size;
        // remaining free space
        chunk = (Header *) new;
        chunk -> status = FREE;
        chunk -> size = fsize - asize;
        // update freeList[]
        freeList[found] = new;
    }
    return new2;
}

// free a chunk of memory
void myFree(void *block)
{
    // TODO
    Addr heapTop = (Addr)((char *)heapMem + heapSize);
    if (block == NULL || block < heapMem || block >= heapTop)
    {
        fprintf(stderr, "Attempt to free unallocated chunk\n");
        exit(1);
    }
    Addr header = (Addr)(char *) block - sizeof(Header);
    Header * head = (Header *) header;
    Header * chunk;
    
    int i = nFree - 1;
    int j = nFree;
    if (head -> status == ALLOC)
    {
        // insert freed chunk into sorted array
        while (i >= 0  && freeList[i] > header)
        {
            freeList[j] = freeList[i];
            i--; j--;
        }
        freeList[j] = header;
        head -> status = FREE;
        nFree ++;
    }
    else
    {
        fprintf(stderr, "Attempt to free unallocated chunk\n");
        exit(1);
    }
    Header * prevC; // ptr to prev chunk
    Header * nextC; // ptr to next chunk
    // if a chunk has free neighbours
    if (nFree > 1)
    {
        i = 0;
        j = 1;
        while(i < nFree)
        {
            Addr curr = freeList[i];
            chunk = (Header *) curr;
            nextC = (Header *) freeList[j];
            
            Addr currNext = (Addr)(char *) freeList[i] + chunk -> size;
            Addr next = freeList[j];
            Addr prev;
            // neighbour is free chunk
            if (currNext == next) // merge chunk with next
            {
                // merge
                chunk -> size += nextC -> size;
                // update freeList[]
                memset(freeList[j], 0, nextC -> size);
                nFree --;
                for (int k = j; k < nFree; k ++)
                {
                    freeList[k] = freeList[k + 1];
                }
            }
            if (i != 0) // merge chunk with prev
            {
                prevC = (Header *) freeList[i - 1];
                prev = (Addr)(char *) freeList[i - 1] + prevC -> size;
                if (prev == curr)
                {
                    prevC -> size += chunk -> size;
                    // update freeList[]
                    memset(freeList[i], 0, chunk -> size);
                    nFree --;
                    for (int k = i; k < nFree; k ++)
                    {
                        freeList[k] = freeList[k + 1];
                    }
                }
            }
            i ++; j ++;
        }
    }
}

// convert pointer to offset in heapMem
int  heapOffset(void *p)
{
    Addr heapTop = (Addr)((char *)heapMem + heapSize);
    if (p == NULL || p < heapMem || p >= heapTop)
        return -1;
    else
        return p - heapMem;
}

// dump contents of heap (for testing/debugging)
void dumpHeap()
{
    Addr    curr;
    Header *chunk;
    Addr    endHeap = (Addr)((char *)heapMem + heapSize);
    int     onRow = 0;
    
    curr = heapMem;
    while (curr < endHeap) {
        char stat;
        chunk = (Header *)curr;
        switch (chunk->status) {
            case FREE:  stat = 'F'; break;
            case ALLOC: stat = 'A'; break;
            default:    fprintf(stderr,"Corrupted heap %08x\n",chunk->status); exit(1); break;
        }
        printf("+%05d (%c,%5d) ", heapOffset(curr), stat, chunk->size);
        onRow++;
        if (onRow%5 == 0) printf("\n");
        curr = (Addr)((char *)curr + chunk->size);
    }
    if (onRow > 0) printf("\n");
}