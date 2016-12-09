// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef DRAWER_BUFFER_H
#define DRAWER_BUFFER_H

#include <stdlib.h>
#include <stdbool.h>

#define DRAWER_BUFFER_SIZE (1<<13)  // columns/rows, used memory: DRAWER_BUFFER_SIZE * ((rows|columns) * 3 + O(1)) B

struct dbufferS {
	size_t columnLen;
	int begin; // inclusive
	int end;   // exclusive
	enum {DBUFF_INVALID=-1, DBUFF_READY=0, DBUFF_PROCESSING} state[DRAWER_BUFFER_SIZE];
	unsigned char precision[DRAWER_BUFFER_SIZE];
	bool drawn[DRAWER_BUFFER_SIZE];
	bool dataInvalid;
	unsigned char *data;
	bool previewCreated[DRAWER_BUFFER_SIZE];
	unsigned char preview[DRAWER_BUFFER_SIZE*3];
	unsigned char *columnOverlay;
};

extern struct dbufferS dbuffer;

extern double dbufferColumnsPerSecond;

#define dbufferColToI(column) (((column) + DRAWER_BUFFER_SIZE) % DRAWER_BUFFER_SIZE)
#define dbuffer(column,pixel,channel) dbuffer.data[(dbufferColToI(column) * dbuffer.columnLen + (pixel)) * 3 + (channel)]
#define dbufferPreview(column,channel) dbuffer.preview[dbufferColToI(column) * 3 + (channel)]
#define dbufferPreviewCreated(column) dbuffer.previewCreated[dbufferColToI(column)]
#define dbufferState(column) dbuffer.state[dbufferColToI(column)]
#define dbufferPrecision(column) dbuffer.precision[dbufferColToI(column)]
#define dbufferDrawn(column) dbuffer.drawn[dbufferColToI(column)]
#define dbufferExists(column) ((dbuffer.begin <= (column)) && (dbuffer.end > (column)))
#define dbufferColumnOverlay(pixel,channel) dbuffer.columnOverlay[(pixel) * 4 + (channel)]


void dbufferInit();
void dbufferRealloc(size_t columnLen); // to be called only when all threads are aware of dataInvalid=true
void dbufferMove(int offset); // to be called only from the main thread

static inline bool dbufferTransactionBegin(int column, unsigned char precision) { // returns whether transaction was started
	if (__sync_bool_compare_and_swap(&dbufferState(column), DBUFF_READY, DBUFF_PROCESSING)) {
		if (dbufferExists(column) && (dbufferPrecision(column)<precision)) {
			return true;
		}
		// state is now DBUFF_PROCESSING or DBUFF_INVALID
		dbufferState(column) = DBUFF_READY;
	}
	return false;
}
static inline void dbufferTransactionAbort(int column) { // aborts active transaction
	dbufferPrecision(column) = 0;
	__sync_synchronize();
	dbufferState(column) = DBUFF_READY;
}
static inline bool dbufferTransactionCheck(int column) { // checks whether transaction is still not aborted
	return (dbufferState(column) == DBUFF_PROCESSING);
}
static inline bool dbufferTransactionCommit(int column, unsigned char precision) { // tries to commit active transaction
	// state is now DBUFF_PROCESSING or DBUFF_INVALID, column exists
	if (__sync_bool_compare_and_swap(&dbufferState(column), DBUFF_PROCESSING, DBUFF_READY)) {
		dbufferPrecision(column) = precision;
		dbufferDrawn(column) = false;
		return true;
	} else {
		// state is now DBUFF_INVALID
		dbufferTransactionAbort(column);
	}
	return false;
}

#endif
