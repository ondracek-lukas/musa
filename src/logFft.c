// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <math.h>
#include <stdbool.h>
#include <float.h>
#include <stdio.h>
#include "fft.h"
#include "logFft.h"
#include "mem.h"
#include "resampler.h"


float minFreq, maxFreq;
int outputLen, blockLen, blockLenLog2;

struct fftSpectrumContext *fftContext = NULL;

extern void logFftReset(
		float newMinFreq,
		float newMaxFreq,
		int newOutputLen,
		int *blockLenOut) {

	minFreq= newMinFreq;
	maxFreq= newMaxFreq;
	outputLen = newOutputLen;

	float blocksPerColumnF = log2f(maxFreq/minFreq);
	blockLenLog2=ceilf(log2f( 2*outputLen/blocksPerColumnF ));
	blockLen = 1<<blockLenLog2;
	*blockLenOut = blockLen;

	fftDestroySpectrumContext(fftContext);
	fftContext = fftCreateSpectrumContext(blockLenLog2);
}


#define columnBlock(i,j) (fftBlocks[(i)*blockLen + (j)])

bool logFftProcess(double posSec, int blocksCnt, float *outputBuffer) {
	static __thread float *fftBlocks     = NULL;
	static __thread size_t fftBlocksSize = 0;
	float *result;

	{
		size_t size = blockLen * blocksCnt * sizeof(float);
		if (fftBlocksSize < size) {
			free(fftBlocks);
			fftBlocks = memMalloc(size);
			fftBlocksSize = size;
		}
	}

	for (int i = 0; i < blocksCnt; i++) {
		if (!rsRead(i, posSec, blockLen/2, &columnBlock(i,0))) {
			return false;
		}
	}
	for (int i = 0; i < blocksCnt; i++) {
		fftSpectrum(&columnBlock(i,0), fftContext);
	}

	double freqMultPerItem = exp2(log2(maxFreq/minFreq)/(outputLen-1));
	double freq = minFreq;
	int last = blocksCnt-1;
	for (int i = 0; i < outputLen; i++) {
		float value = FLT_MAX;
		for (int j = last; j >= 0; j--) {
			// curBlockMaxFreq: 1.0 >> j
			double indexD = blockLen * freq / rsRates[j];
			int index = indexD; indexD -= index;
			if (index*2 >= blockLen) {
				last--;
				continue;
			}

			float newValue=0;
			newValue += columnBlock(j, index) * (1 - indexD);
			newValue += columnBlock(j, index + 1) * indexD;

			if (newValue < value) {
				value = newValue;
			}
		}
		if (value == FLT_MAX) {
			outputBuffer[i] = 0;
		} else {
			outputBuffer[i] = value;
		}
		freq *= freqMultPerItem;
	}

	return true;
}
