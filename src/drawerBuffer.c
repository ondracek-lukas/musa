// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include "drawerBuffer.h"


struct dbufferS dbuffer;
double dbufferColumnsPerSecond=0;


void dbufferRealloc(size_t columnLen) { // to be called only when all threads are aware of dataInvalid=true
	dbuffer.columnLen=columnLen;
	free(dbuffer.data);
	dbuffer.data = calloc(columnLen*DRAWER_BUFFER_SIZE*3, sizeof(unsigned char));
	for (int i=dbuffer.begin; i<dbuffer.end; i++) {
		dbufferState(i) = DBUFF_EMPTY;
	}
	__sync_synchronize();
	dbuffer.dataInvalid=false;
	__sync_synchronize();
}
void dbufferMove(int offset) { // to be called only from the main thread
	if (offset>0) {
		dbuffer.begin += offset;
		__sync_synchronize();
		for (int i=dbuffer.end; i<dbuffer.end+offset; i++) {
			if (__sync_lock_test_and_set(&dbufferState(i), DBUFF_INVALID) != DBUFF_PROCESSING)
				dbufferState(i)=DBUFF_EMPTY;
			dbufferPreviewCreated(i)=false;
		}
		__sync_synchronize();
		dbuffer.end += offset;
		__sync_synchronize();
	} else {
		dbuffer.end -= offset;
		__sync_synchronize();
		for (int i=dbuffer.begin-offset; i<dbuffer.begin; i++) {
			if (__sync_lock_test_and_set(&dbufferState(i), DBUFF_INVALID) != DBUFF_PROCESSING)
				dbufferState(i)=DBUFF_EMPTY;
			dbufferPreviewCreated(i)=false;
		}
		__sync_synchronize();
		dbuffer.begin -= offset;
	}
}
