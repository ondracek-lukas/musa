#include "resampler.h"

#include <stdlib.h>
#include <math.h>
#include "messages.h"
#include "streamBuffer.h"
#include "player.h"
#include "util.h"
#include "taskManager.h"
#include "fft.h"

#define FFT_BLOCK_LEN_LOG2 8
#define IMPULSE_RESPONSE_LEN 129  // assert: is odd

#define FFT_BLOCK_LEN    (1<<FFT_BLOCK_LEN_LOG2)
#define BLOCK_LEN        ((FFT_BLOCK_LEN-IMPULSE_RESPONSE_LEN+1)/2)
#define BLOCKS_PER_TASK  4

int rsCnt = 0;
double *rsRates = NULL;
struct streamBuffer **rsBuffers = NULL;
static int *writeLocks = NULL;

struct taskInfo rsTask = TM_TASK_INITIALIZER(false, false);

static struct fftFilterContext *filterContext = NULL;

static bool taskFunc();
static void __attribute__((constructor)) init() {
	tmTaskRegister(&rsTask, taskFunc, 2);
}

void rsReset() {
	int newCnt = ceilf(-log2f(msgOption_minFreq/playerSampleRate) - 1);
	if (newCnt < 1) newCnt = 1;
	for (int i = newCnt; i < rsCnt; i++) {
		free(rsBuffers[i]);
	}
	rsBuffers  = utilRealloc(rsBuffers,  newCnt*sizeof(struct streamBuffer *));
	rsRates    = utilRealloc(rsRates,    newCnt*sizeof(double));
	writeLocks = utilRealloc(writeLocks, newCnt*sizeof(int));
	
	if (rsCnt == 0) {
		rsBuffers[0] = &playerBuffer;
		rsRates[0]   = playerSampleRate;
		rsCnt = 1;
	}
	for (int i = rsCnt; i < newCnt; i++) {
		rsBuffers[i] = utilMalloc(sizeof(struct streamBuffer));
		int streamBegin = rsBuffers[i-1]->streamBegin;
		if (streamBegin != -1) {
			streamBegin /= 2;
			//streamBegin -= impulseResponseSize
		}
		int streamEnd = rsBuffers[i-1]->streamEnd;
		if (streamEnd != -1) {
			streamEnd /= 2;
			//streamEnd += impulseResponseSize;
		}
		sbReset(rsBuffers[i], 0, streamBegin, streamEnd);
		rsRates[i]   = rsRates[i-1]/2;
		writeLocks[i] = 0;
	}
	rsCnt = newCnt;
	rsTask.active = (rsCnt > 1);

	fftDestroyFilterContext(filterContext);
	float ir[IMPULSE_RESPONSE_LEN];
	ir[IMPULSE_RESPONSE_LEN/2] = 1.0/2;  // assert: IMPULSE_RESPONSE_LEN is odd
	for (int i = 1; i <= IMPULSE_RESPONSE_LEN/2; i++) {
		ir[IMPULSE_RESPONSE_LEN/2 - i] = ir[IMPULSE_RESPONSE_LEN/2 + i] = sin(M_PI_2*i)/(M_PI_2*i)/2;
	}
	
	filterContext = fftCreateFilterContext(ir, IMPULSE_RESPONSE_LEN, FFT_BLOCK_LEN_LOG2);
}

int rsContainsCnt(double posSec, int radius) {
	int i;
	for (i = 0; i < rsCnt; i++) {
		int pos = posSec * rsRates[i];
		if (!sbContainsBegin(rsBuffers[i], pos-radius) || !sbContainsEnd(rsBuffers[i], pos+radius)) {
			break;
		}
	}
	return i;
}
bool rsRead(int index, double posSec, int radius, float *outBuffer) {
	int pos = posSec * rsRates[index];
	return sbRead(rsBuffers[index], pos-radius, pos+radius, outBuffer);
}

static inline bool tryProcess(int i, bool forward) {
	int begin, end;
	if (forward) {
		if (sbAtStreamEnd(rsBuffers[i])) {
			return false;
		}
		begin = rsBuffers[i]->end;
		end   = begin + BLOCK_LEN;
	} else {
		if (sbAtStreamBegin(rsBuffers[i])) {
			return false;
		}
		end   = rsBuffers[i]->begin;
		begin = end - BLOCK_LEN;
	}
	
	float buffer[FFT_BLOCK_LEN];
	if (!sbRead(rsBuffers[i-1], 2*begin-IMPULSE_RESPONSE_LEN/2, 2*end+IMPULSE_RESPONSE_LEN/2, buffer)) {
		return false;
	}

	fftFilter(buffer, filterContext);

	if (forward) {
		sbPreAppend(rsBuffers[i], end);
	} else {
		sbPrePrepend(rsBuffers[i], begin);
	}
	
	for (int j = begin, j2=IMPULSE_RESPONSE_LEN/2; j < end; j++, j2+=2) {
		sbValue(rsBuffers[i], j) = buffer[j2];
	}
	
	if (forward) {
		sbPostAppend(rsBuffers[i], end);
	} else {
		sbPostPrepend(rsBuffers[i], begin);
	}
	return true;
}

static bool taskFunc() {
	double visibleBeginSec = playerPos/playerSampleRate - msgOption_visibleBefore/msgOption_outputRate;
	double visibleEndSec   = playerPos/playerSampleRate + msgOption_visibleAfter/msgOption_outputRate;

	int processedCnt = 0;
	for (int i = rsCnt-1; (i > 0) && (processedCnt < BLOCKS_PER_TASK); i--) {
		if (!__sync_bool_compare_and_swap(&writeLocks[i], 0, 1)) continue;
		int visibleBegin = visibleBeginSec * rsRates[i];
		int visibleEnd   = visibleEndSec   * rsRates[i];
		if (!sbContainsBegin(rsBuffers[i], visibleEnd) && !sbContainsEnd(rsBuffers[i], visibleBegin)) {
			sbReset(rsBuffers[i], (visibleBegin+visibleEnd)/2, rsBuffers[i]->streamBegin, rsBuffers[i]->streamEnd);
			printf("resampler: %d reset\n", i); // XXX
		}
		bool forwardPreferred = (visibleBegin - rsBuffers[i]->begin > rsBuffers[i]->end - visibleEnd);
		for (; processedCnt < BLOCKS_PER_TASK; processedCnt++) {
			if (tryProcess(i,  forwardPreferred)) continue;
			if (tryProcess(i, !forwardPreferred)) continue;
			break;
		}
		writeLocks[i] = 0;
	}
	return processedCnt;
}
