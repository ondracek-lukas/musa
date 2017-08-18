// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <math.h>
#include <float.h>
#include <limits.h>
#include <string.h>
#include "logFft.h"
#include "drawer.h"
#include "drawerScale.h"
#include "drawerBuffer.h"
#include "player.h"
#include "util.h"
#include "taskManager.h"
#include "messages.h"
#include "resampler.h"
#include "overtoneFilter.h"

#define MAX_PRECISION_LEVELS 32

static int
	 processedFrom[MAX_PRECISION_LEVELS],
	 processedTo[MAX_PRECISION_LEVELS];
#define PROCESSED_UPDATE_PERIOD 16 // update processedFrom, processedTo every PERIOD success
static unsigned char minPrecision, maxPrecision;
static int fftBlockLen;


struct taskInfo dmvTask = TM_TASK_INITIALIZER(false, false);


// --- COLORING ---

static unsigned char colorScale[1024*3]={255}; // will be initialized below

static inline void toColorScale(float value, unsigned char *color) {
	unsigned valueI = value*1024;
	if (valueI>=1024)
		valueI=1023;
	valueI*=3;

	color[0] = colorScale[valueI];
	color[1] = colorScale[valueI+1];
	color[2] = colorScale[valueI+2];
}


static void createColumn(int column, float *logFftResult) { // assert: logFftResult can be modified
	if (msgOption_filterOvertones) {
		ofProcess(logFftResult);
	}

	double maxAmp = 0.0001;
	double minAmp = logFftResult[0];
	for (int i=0; i<dbuffer.columnLen; i++) {
		if (maxAmp < logFftResult[i]) maxAmp = logFftResult[i];
		if (minAmp > logFftResult[i]) minAmp = logFftResult[i];
	}


	for (int i=0; i<dbuffer.columnLen; i++) {
		double amp = logFftResult[i];

		if (msgOption_dynamicGain) {
			amp = log10(amp/maxAmp);
		} else {
			amp = log10(amp) + msgOption_gain*0.1;
		}
		amp = amp / (0.1*msgOption_signalToNoise) + 1;

		if      (amp > 1) amp = 1;
		else if (amp < 0) amp = 0;

		toColorScale(amp, &dbuffer(column, i, 0));
	}
}



// --- INIT FUNCTIONS ---

static bool taskFunc();
static __attribute__((constructor)) void init() {
	tmTaskRegister(&dmvTask, taskFunc, 1);

	// colorScale init
	size_t i=0;
	for (; i<64; i++) {
		colorScale[3*i]  =0;
		colorScale[3*i+1]=0;
		colorScale[3*i+2]=sqrt(i)*16;
	}
	for (; i<144; i++) {
		colorScale[3*i]  =sqrt(i)*32-256;
		colorScale[3*i+1]=0;
		colorScale[3*i+2]=128;
	}
	for (; i<256; i++) {
		colorScale[3*i]  =sqrt(i)*32-256;
		colorScale[3*i+1]=0;
		colorScale[3*i+2]=511-sqrt(i)*32;
	}
	for (; i<1024; i++) {
		colorScale[3*i]  =255;
		colorScale[3*i+1]=sqrt(i)*16-256;
		colorScale[3*i+2]=0;
	}
}


static void onRestart();

void dmvReset() { // pause dmvTask and drawerMainTask before
	if (playerSourceType == PLAYER_SOURCE_NONE) {
		dmvTask.active = false;
		return;
	}
	if ((msgOption_columnLen<=0) || (msgOption_minFreq > msgOption_maxFreq)) { // XXX check elsewhere
		dmvTask.active = false;
		return;
	}
	dbufferRealloc(msgOption_columnLen);
	int pos = playerPosSec * msgOption_outputRate;
	for (int i=0; i<MAX_PRECISION_LEVELS; i++) {
		processedTo[i] = processedFrom[i] = pos;
	}

	logFftReset(
			msgOption_minFreq,
			msgOption_maxFreq,
			msgOption_columnLen, &fftBlockLen);
	minPrecision = 1;
	maxPrecision = rsCnt;
	dmvTask.active = true;
}


// --- DATA PROCESSING ---


static inline unsigned char dlog2(unsigned int n) {
	if (!n) return 0;
	unsigned char l=-1;
	for (; n; n >>= 1) l++;
	return l;
}

static inline bool tryProcessLogFft(
		int pos,
		float *outputBuffer,
		unsigned char *precisionPtr) {
	double posSec = pos / msgOption_outputRate;
	unsigned char precision = rsContainsCnt(posSec, fftBlockLen/2);

	if (precision < *precisionPtr) {
		*precisionPtr = precision;
	}

	if (dbufferTransactionBegin(pos, precision)) {
		if (logFftProcess(posSec, precision, outputBuffer)) {
			*precisionPtr = precision;
			return true;
		} else {
			dbufferTransactionAbort(pos);
		}
	}
	return false;
}

static bool taskFunc() {
	static int locked = 0;
#define TRY_LOCK __sync_bool_compare_and_swap(&locked, 0, 1)
#define UNLOCK locked = 0


	int bufferPos = playerPosSec * msgOption_outputRate;
	int visibleTo = drawerVisibleEnd * msgOption_outputRate;
	int visibleFrom = drawerVisibleBegin * msgOption_outputRate;
	int visibleArea = visibleTo - visibleFrom;

	{
		int moveBy = (visibleFrom + visibleTo) / 2 - (dbuffer.begin + dbuffer.end) / 2;
		if ((abs(moveBy) > DRAWER_BUFFER_SIZE/8) && TRY_LOCK) {
			moveBy = (visibleFrom + visibleTo) / 2 - (dbuffer.begin + dbuffer.end) / 2;
			if (abs(moveBy) > DRAWER_BUFFER_SIZE/8) {
				dbufferMove(moveBy);
			}
			UNLOCK;
		}
	}

	if (visibleTo > dbuffer.end) visibleTo = dbuffer.end;
	if (visibleFrom < dbuffer.begin) visibleFrom = dbuffer.begin;
	if (visibleFrom < 0) visibleFrom = 0;

	if (visibleTo < visibleFrom) {
		return false;
	}

	// update processed
	if (((visibleTo < processedFrom[maxPrecision]) && (bufferPos + visibleArea/2 < processedFrom[maxPrecision])) ||
			((visibleFrom > processedTo[maxPrecision]) && (bufferPos - visibleArea/2 > processedTo[maxPrecision]))) {
		if (TRY_LOCK) {
			for (int p = minPrecision; p <= maxPrecision; p++) {
				processedFrom[p] = processedTo[p] = bufferPos;
			}
			UNLOCK;
		}
	}

	// XXX  optimize the function till this point

	bool logFftProcessed = false;
	static __thread float *buffer = NULL;
	{
		static __thread int bufferSize = 0;
		if (bufferSize != dbuffer.columnLen) {
			free(buffer);
			bufferSize = dbuffer.columnLen;
			buffer = utilMalloc(bufferSize * sizeof(float));
		}
	}


	int pos;
	unsigned char precision;

	if (!logFftProcessed) {
		precision = maxPrecision;
		for (pos = processedTo[maxPrecision]; pos < visibleTo; pos++) {
			if (tryProcessLogFft(pos, buffer, &precision)) {
				logFftProcessed = true;
				break;
			} else if (precision < minPrecision) {
				break;
			} else if (processedTo[precision] > pos) {
				pos = processedTo[precision]-1;
			}
		}
	}

	if (!logFftProcessed) {
		precision = maxPrecision;
		for (pos = processedFrom[maxPrecision]-1; (pos >= visibleFrom) && (pos >= 0); pos--) {
			if (tryProcessLogFft(pos, buffer, &precision)) {
				logFftProcessed = true;
				break;
			} else if (precision < minPrecision) {
				break;
			} else if (processedFrom[precision] < pos) {
				pos = processedFrom[precision]+1;
			}
		}
	}

	if (!logFftProcessed) {
		precision = maxPrecision;
		for (pos = (visibleTo>processedTo[maxPrecision] ? visibleTo : processedTo[maxPrecision]); pos < dbuffer.end; pos++) {
			if (tryProcessLogFft(pos, buffer, &precision)) {
				logFftProcessed = true;
				break;
			} else if (precision < minPrecision) {
				break;
			} else if (processedTo[precision] > pos) {
				pos = processedTo[precision]-1;
			}
		}
	}

	if (!logFftProcessed) {
		precision = maxPrecision;
		for (pos = (visibleFrom<processedFrom[maxPrecision] ? visibleFrom : processedFrom[maxPrecision])-1; (pos >= dbuffer.begin) && (pos >= 0); pos--) {
			if (tryProcessLogFft(pos, buffer, &precision)) {
				logFftProcessed = true;
				break;
			} else if (precision < minPrecision) {
				break;
			} else if (processedFrom[precision] < pos) {
				pos = processedFrom[precision]+1;
			}
		}
	}

	if (!logFftProcessed) {
		return false;
	}

	if (!dbufferTransactionCheck(pos)) {
		dbufferTransactionAbort(pos);
		return false;
	}

	createColumn(pos, buffer);

	bool success = dbufferTransactionCommit(pos, precision);
	if (success) {
		static int counter = 0;
		if (__sync_add_and_fetch(&counter, 1) % PROCESSED_UPDATE_PERIOD == 0) {
			if (TRY_LOCK) {
				int p    = maxPrecision;
				int to   = processedTo[p];
				int from = processedFrom[p];
				for (; p >= minPrecision; p--) {
					for (; dbufferExists(to)   && (dbufferPrecision(to)   >= p); to++);
					processedTo[p]   = to;
					for (; dbufferExists(from) && (dbufferPrecision(from) >= p); from--);
					processedFrom[p] = from;
				}
				UNLOCK;
			}
		}
	}
	return success;
}
