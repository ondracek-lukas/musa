// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <math.h>
#include <stdbool.h>
#include <float.h>
#include "fft.h"
#include "logFft.h"
#include "util.h"

static logFftDataExchangeCallbackT *exchangeCallback = NULL;
static void (*restartCallback)() = NULL;

float minFreqMult, maxFreqMult;
int outputLen, blockLen, blockLenLog2, firstBlock, lastBlock;

struct column {
	void *info;
	int blocksProcessed;
	int blocksCnt;
	float *fftBlocks;
	float *result;
};

extern void logFftInit() {
	fftInit();
}

float *avgMagRepairTbl=NULL;
#define avgMagRepairTbl(i,j) (avgMagRepairTbl[((i)-(firstBlock?firstBlock:1))*blockLen/2 + (j)-1])

static float *onDataExchange(void **info, bool restartNeeded);
extern void logFftStart(
		float newMinFreqMult,
		float newMaxFreqMult,
		int newOutputLen,
		logFftDataExchangeCallbackT newExchangeCallback,
		unsigned char *minPrecision, unsigned char *maxPrecision) {

	minFreqMult = newMinFreqMult;
	maxFreqMult = newMaxFreqMult;
	outputLen = newOutputLen;
	firstBlock = floorf(-log2f(maxFreqMult) - 1);
	lastBlock  =  ceilf(-log2f(minFreqMult) - 1) -1;
	exchangeCallback=newExchangeCallback;

	float blocksPerColumnF = log2f(maxFreqMult/minFreqMult);
	blockLenLog2=ceilf(log2f( 2*outputLen/blocksPerColumnF ));
	blockLen = 1<<blockLenLog2;

	*minPrecision = blockLenLog2 - 1 + firstBlock; // log of neighbourhood needed
	*maxPrecision = blockLenLog2 - 1 + lastBlock;

	printf("blockLen: %d, blocks: %d, prec: %d--%d\n", blockLen, lastBlock+1, *minPrecision, *maxPrecision);

	free(avgMagRepairTbl);
	avgMagRepairTbl = utilMalloc((lastBlock-(firstBlock?firstBlock:1)+1) * blockLen/2 * sizeof(float));
	for (int i = (firstBlock?firstBlock:1); i <= lastBlock; i++) {
		double cosStep = M_PI / (1<<i) / blockLen;
		for (int j = 1; j <= blockLen/2; j++) {
			double value=0;
			for (int k = 1; k < (1<<i); k += 2) {
				value += cos(cosStep * k * j);
			}
			value = (1<<(i-1))/value;
			avgMagRepairTbl(i,j) = value;
		}
	}

	fftStart(blockLenLog2, onDataExchange);
}


#define columnBlock(i,j) (column->fftBlocks[((i)-firstBlock)*blockLen + (j)])
static float *onDataExchange(void **info, bool restarted) {
	float *resultData=NULL;
	struct column *column = *info;
	if (restarted) {
		if (column) {
			free(column->fftBlocks);
			free(column->result);
		} else {
			column = utilMalloc(sizeof(struct column));
		}
		column->info = NULL;
		column->fftBlocks = utilMalloc(blockLen * (lastBlock-firstBlock+1) * sizeof(float));
		column->result = utilMalloc(outputLen * sizeof(float));
		*info = column;
	} else if (column->blocksProcessed >= 0) {
		if (column->blocksProcessed > 0) {
			int i=column->blocksProcessed + firstBlock;
			for (int j = 1; j <= blockLen/2; j++) {
				columnBlock(i,j) *= avgMagRepairTbl(i,j);
			}
		}
		if (++column->blocksProcessed < column->blocksCnt) {
			return column->fftBlocks + column->blocksProcessed * blockLen;
		} else {
			resultData = column->result;
			double freqMultPerItem = exp2(log2(maxFreqMult/minFreqMult)/(outputLen-1));
			float freq = minFreqMult;
			int last = firstBlock + column->blocksCnt - 1;
			for (int i = 0; i < outputLen; i++) {
				float value = FLT_MAX;
				for (int j = last; j >= firstBlock; j--) {
					// curBlockMaxFreq: 1.0 >> j
					float indexF = blockLen * freq * (1 << j);
					int index = indexF; indexF -= index;
					if (index*2 >= blockLen) {
						last--;
						continue;
					}

					float newValue=0;
					newValue += columnBlock(j, index) * (1 - indexF);
					newValue += columnBlock(j, index + 1) * indexF;

					if (newValue < value) {
						value = newValue;
					}

					//if (j > last) break;
				}
				resultData[i] = value;
				freq *= freqMultPerItem;
			}
			//exit(42);
		}
	}

	int pos;
	int halfBlockLen = blockLen/2;
	float avgs[lastBlock+1];
	column->blocksCnt = 0;
	void loadInput(unsigned char precision, float *data, float *dataEnd) {
		if (column->blocksCnt != precision - blockLenLog2 + 1 - firstBlock + 1) { // clear
			column->blocksCnt = precision - blockLenLog2 + 1 - firstBlock + 1;
			pos = -1 << precision;
		}
		int last = firstBlock + column->blocksCnt - 1;
		for (; data<dataEnd; data++, pos++) {
			avgs[0] = *data;
			for (int i=0, p=pos; ; i++, p>>=1) {
				if ((p >= -halfBlockLen) && (p < halfBlockLen) && (i >= firstBlock)) {
					columnBlock(i, p+halfBlockLen) = avgs[i];
				}
				if (i >= last) break;
				if (!(p&1)) {
					avgs[i+1] = avgs[i];
					break;
				} else {
					avgs[i+1] = (avgs[i+1] + avgs[i]) / 2;
				}
			}
		}
	}
	if (!exchangeCallback(&column->info, resultData, loadInput)) {
		column->blocksProcessed=-1;
		return NULL;
	}
	column->blocksProcessed=0;
	return column->fftBlocks;
}
