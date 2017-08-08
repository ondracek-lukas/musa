// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include "drawerBuffer.h"


struct dbufferS dbuffer;


void dbufferInit() {
	dbuffer.columnLen=0;
	dbuffer.begin=-DRAWER_BUFFER_SIZE;
	dbuffer.end=0;
	dbuffer.dataInvalid=true;
}
void dbufferRealloc(size_t columnLen) {
	dbuffer.columnLen=columnLen;
	free(dbuffer.data);
	dbuffer.data = calloc(columnLen*DRAWER_BUFFER_SIZE*3, sizeof(unsigned char));
	dbuffer.columnOverlay = calloc(columnLen*4, sizeof(unsigned char));
	for (int i=dbuffer.begin; i<dbuffer.end; i++) {
		dbufferState(i) = DBUFF_READY;
		dbufferPrecision(i) = 0;
		dbufferDrawn(i) = false;
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
				dbufferState(i)=DBUFF_READY;
			dbufferDrawn(i)=false;
			dbufferPrecision(i)=0;
		}
		__sync_synchronize();
		dbuffer.end += offset;
		__sync_synchronize();
	} else {
		dbuffer.end += offset;
		__sync_synchronize();
		for (int i=dbuffer.begin+offset; i<dbuffer.begin; i++) {
			if (__sync_lock_test_and_set(&dbufferState(i), DBUFF_INVALID) != DBUFF_PROCESSING)
				dbufferState(i)=DBUFF_READY;
			dbufferDrawn(i)=false;
			dbufferPrecision(i)=0;
		}
		__sync_synchronize();
		dbuffer.begin += offset;
	}
}
