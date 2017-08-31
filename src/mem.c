// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include "mem.h"
#include "util.h"
#include <stdlib.h>


// --- stdlib memory alloc wrappers ---

void *memMalloc(size_t size) {
	void *mem = malloc(size);
	if (size && !mem) {
		utilExitErr("Out of memory (%llu B needed)", size);
	}
	return mem;
}

void *memCalloc(size_t nmemb, size_t size) {
	void *mem=calloc(nmemb, size);
	if (size && nmemb && !mem)
		utilExitErr("Out of memory");
	return mem;
}

void *memRealloc(void *ptr, size_t size) {
	void *mem=realloc(ptr, size);
	if (size && !mem)
		utilExitErr("Out of memory");
	return mem;
}


// --- thread-local storage allocation ---

static size_t memPerThreadCnt = 0;
struct memPerThread {
	size_t index;
	size_t size;
	struct memPerThread *next;
};
static struct memPerThread *freed = NULL;

struct memPerThread *memPerThreadMalloc(size_t size) {
	struct memPerThread *mpt;
	while ((mpt = freed) && !__sync_bool_compare_and_swap(&freed, mpt, mpt->next));
	if (!mpt) {
		mpt = memMalloc(sizeof(struct memPerThread));
		mpt->index = __sync_fetch_and_add(&memPerThreadCnt, 1);
	}
	mpt->size  = size;
	return mpt;
}

void memPerThreadFree(struct memPerThread *mpt) {
	do {
		mpt->next = freed;
	} while (!__sync_bool_compare_and_swap(&freed, mpt->next, mpt));
}


static __thread int threadDataCnt = 0;
static __thread struct threadData {
	size_t size;
	void *mem;
} *threadData = NULL;

void *memPerThreadGet(struct memPerThread *mpt) {
	if (mpt->index >= threadDataCnt) {
		threadData    = memRealloc(threadData, sizeof(struct threadData) * (mpt->index+1));
		for (; threadDataCnt <= mpt->index; threadDataCnt++) {
			threadData[threadDataCnt].size = 0;
			threadData[threadDataCnt].mem  = NULL;
		}
	}
	if (threadData[mpt->index].size != mpt->size) {
		free(threadData[mpt->index].mem);
		threadData[mpt->index].size = mpt->size;
		threadData[mpt->index].mem  = memMalloc(mpt->size);
	}
	return threadData[mpt->index].mem;
}


// -- string alloc --

static char *strReallocPtrOld=0;
static char *strReallocPtrNew=0;
void memStrRealloc(char **ptr, char **ptr2, size_t minSize) {
	// *ptr is pointer to the block
	// *ptr2 is another pointer in the block or ptr2==0
	// minSize is required size from *ptr2 or *ptr if not set
	static struct blocks {
		char *ptr;
		size_t size;
		struct blocks *next;
	} *blocks=0;
	struct blocks *block, **pBlock;

	strReallocPtrOld=*ptr;
	if (!*ptr) {
		block=memMalloc(sizeof(struct blocks));
		block->size=minSize;
		block->ptr=*ptr=memMalloc(minSize*sizeof(char));
		block->next=blocks;
		blocks=block;
	} else {
		for (pBlock=&blocks; *pBlock; pBlock=&(*pBlock)->next)
			if ((*pBlock)->ptr == *ptr)
				break;
		if (!(block=*pBlock))
			utilExitErr("String manipulation error");
		if (ptr2)
			minSize+=*ptr2-*ptr;
		if (minSize==0) {
			free(*ptr);
			*ptr=0;
			*pBlock=block->next;
			free(block);
		} else if (block->size<minSize) {
			block->size=16;
			while (block->size<minSize) {
				block->size*=2;
				if (block->size==0)
					utilExitErr("Too much memory needed");
			}
			block->ptr=memRealloc(block->ptr, block->size*sizeof(char));
			*ptr=block->ptr;
		}
	}
	strReallocPtrNew=*ptr;
	if (ptr2)
		memStrReallocPtrUpdate(ptr2);
}

void memStrReallocPtrUpdate(char **ptr) {
	*ptr=strReallocPtrNew+(*ptr-strReallocPtrOld);
}

