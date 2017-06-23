// MusA  Copyright (C) 2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <math.h>
#include <stdbool.h>
#include <float.h>
#include <stdio.h>
#include "fft.h"
#include "logFft.h"
#include "util.h"


float minFreqMult, maxFreqMult;
int outputLen, blockLen, blockLenLog2, firstBlock, lastBlock;


float *avgMagRepairTbl=NULL;
#define avgMagRepairTbl(i,j) (avgMagRepairTbl[((i)-(firstBlock?firstBlock:1))*blockLen/2 + (j)-1])

extern void logFftReset(
		float newMinFreqMult,
		float newMaxFreqMult,
		int newOutputLen,
		unsigned char *minPrecision, unsigned char *maxPrecision) {

	minFreqMult = newMinFreqMult;
	maxFreqMult = newMaxFreqMult;
	outputLen = newOutputLen;
	firstBlock = floorf(-log2f(maxFreqMult) - 1);
	lastBlock  =  ceilf(-log2f(minFreqMult) - 1) -1;

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

	fftReset(blockLenLog2);
}


#define columnBlock(i,j) (fftBlocks[((i)-firstBlock)*blockLen + (j)])

void logFftProcess(unsigned char precision, float *inputBuffer, float *outputBuffer) {
	int blocksCnt;
	float *fftBlocks;
	float *result;

	fftBlocks = utilMalloc(blockLen * (lastBlock-firstBlock+1) * sizeof(float));


	int pos;
	int halfBlockLen = blockLen/2;
	float avgs[lastBlock+1];
	blocksCnt = precision - blockLenLog2 + 1 - firstBlock + 1;
	pos = -1 << precision;

	int last = firstBlock + blocksCnt - 1;

	for (float *data = inputBuffer, *dataEnd = inputBuffer+(2<<precision); data<dataEnd; data++, pos++) {
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


	for (int i = firstBlock; i<=lastBlock; i++) {
		fftProcess(&columnBlock(i,0));
	}


	for (int i = firstBlock+1; i<=lastBlock; i++) {
		for (int j = 1; j <= blockLen/2; j++) {
			columnBlock(i,j) *= avgMagRepairTbl(i,j);
		}
	}

	double freqMultPerItem = exp2(log2(maxFreqMult/minFreqMult)/(outputLen-1));
	float freq = minFreqMult;
	for (int i = 0; i < outputLen; i++) {
		float value = FLT_MAX;
		for (int j = last; j >= firstBlock; j--) {
			// curBlockMaxFreq: 1.0 >> j
			float indexF = blockLen * freq * (1 << j);
			int index = indexF; indexF -= index;
			if (index*2 > blockLen) { // maybe >= ?
				last--;
				continue;
			}

			float newValue=0;
			newValue += columnBlock(j, index) * (1 - indexF);
			newValue += columnBlock(j, index + 1) * indexF;

			if (newValue < value) {
				value = newValue;
			}

			// if (j > last) break;
		}
		outputBuffer[i] = value;
		freq *= freqMultPerItem;
	}
	free(fftBlocks);

}
