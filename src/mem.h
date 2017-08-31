// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef MEM_H
#define MEM_H

#include <stdlib.h>


// Wrappers around stdlib functions, guarantee success (or exit app)
void *memMalloc(size_t size);
void *memCalloc(size_t nmemb, size_t size);
void *memRealloc(void *ptr, size_t size);


// Per thread memory allocation
// *Malloc/*Free should be called once, *Get returns thread-local memory
struct memPerThread;
struct memPerThread *memPerThreadMalloc(size_t size);
void memPerThreadFree(struct memPerThread *mpt);
void *memPerThreadGet(struct memPerThread *mpt);


// Reallocs ptr the way that from ptr2 (another pointer in ptr block) is minSize bytes available
// *ptr is pointer to the block
// *ptr2 is another pointer in the block or ptr2==0
// minSize is required size from *ptr2 or *ptr if not set
// both ptr and ptr2 can be changed
extern void memStrRealloc(char **ptr, char **ptr2, size_t minSize);

// Updates poiner ptr in last realloced block after last calling of memStrRealloc
extern void memStrReallocPtrUpdate(char **ptr);


#endif
