// Spectrograph  Copyright (C) 2015  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <float.h>

#include "fft.h"

#define SRC_BLOCK_LEN 256

static bool normalizeAmps;
static float *srcBuffer, *dstBuffer;
static size_t
	*srcRFrom, srcRFree, srcW, dstR, *dstW, dstWFree,
	srcLen,
	dstBlockLen, dstBlocksCnt,
	fftBlockLen,
	fftBlockLenLog2, fftShiftPerBlock,
	fftOutFrom, fftOutTo;
#define dstBuffer(block,index) dstBuffer[(block)*dstBlockLen+(index)]

#define fftWindowGenHamming(x,k) ((k)-(1-(k))*cos(2*M_PI*(x)/fftBlockLen)) // Generalized Hamming window
#define fftWindowHann(x)         fftWindowGenHamming((x),0.5)              // Hann window
#define fftWindowHamming(x)      fftWindowGenHamming((x),0.54)             // Hamming window
#define fftWindowRect(x)         1                                         // Rectangular window
#define fftWindowTrgl(x)         ((float)((x)>fftBlockLen/2?fftBlockLen-(x):(x))*2/fftBlockLen) // Triangular window

#define fftWindow fftWindowHann
static struct cache {
	unsigned reversedIndex;
	float windowValue;
} *cache=0;

static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

static pthread_t srcReaderThread;
static pthread_cond_t srcReaderCond = PTHREAD_COND_INITIALIZER; // srcReader waits for srcBuffer to be freed
static bool srcReaderEOF;

unsigned workersCnt;
static struct workerInfo {
	unsigned id;
	pthread_t threadID;
} *workers;
static pthread_cond_t fftSrcCond = PTHREAD_COND_INITIALIZER; // fftWorker waits for srcBuffer to be filled
static pthread_cond_t fftDstCond = PTHREAD_COND_INITIALIZER; // fftWorker waits for dstBuffer to be freed

static void *srcReader(void *sourceFile);
static void *fftWorker(void *null);

static unsigned runningCnt=0;

static inline size_t reverseLowerBits(size_t value, size_t cnt) {
	value = ((value & 0b1010101010101010) >> 1) | ((value & 0b0101010101010101) << 1);
	value = ((value & 0b1100110011001100) >> 2) | ((value & 0b0011001100110011) << 2);
	value = ((value & 0b1111000011110000) >> 4) | ((value & 0b0000111100001111) << 4);
	value = ((value & 0b1111111100000000) >> 8) | ((value & 0b0000000011111111) << 8);
	return value >> (16-cnt);
}

static void cacheInit() {
	cache=malloc(sizeof(struct cache)*fftBlockLen);
	for (size_t i=0; i<fftBlockLen; i++) {
		size_t j=reverseLowerBits(i,fftBlockLenLog2);
		cache[i].reversedIndex=j;
		cache[i].windowValue=fftWindow(j);
	}
}

void fftStart(FILE *source, size_t blockLenLog2, size_t shiftPerBlock, size_t outputFromIndex, size_t outputToIndex, bool normalizeAmplitudes, unsigned threads) {
	workersCnt=threads;
	srcRFrom=calloc(workersCnt,sizeof(size_t));
	srcRFree=0;
	srcW=0;
	dstR=0;
	dstW=calloc(workersCnt,sizeof(size_t));
	fftBlockLenLog2=blockLenLog2;
	fftBlockLen=1<<blockLenLog2;
	fftShiftPerBlock=shiftPerBlock;
	fftOutFrom=outputFromIndex;
	fftOutTo=outputToIndex;
	dstBlockLen=outputToIndex-outputFromIndex+1;
	dstBlocksCnt=128;
	srcLen=fftBlockLen*8;
	srcBuffer=malloc(sizeof(float)*srcLen);
	dstBuffer=malloc(sizeof(float)*dstBlockLen*dstBlocksCnt);
	srcReaderEOF=false;
	normalizeAmps=normalizeAmplitudes;
	cacheInit();
	workers=malloc(sizeof(struct workerInfo)*workersCnt);
	pthread_create(&srcReaderThread, NULL, srcReader, source);
	for (size_t i=0; i<workersCnt; i++) {
		workers[i].id=i;
		pthread_create(&workers[i].threadID, NULL, fftWorker, workers+i);
	}
}

size_t fftGetData(float *ptr, size_t maxBlocksCnt) {
	size_t cnt=maxBlocksCnt;
	pthread_mutex_lock(&mutex);
	size_t i=dstR;
	for (size_t i=0; i<workersCnt; i++) {
		size_t lcnt=(dstBlocksCnt+dstW[i]-dstR)%dstBlocksCnt;
		if (cnt>lcnt)
			cnt=lcnt;
	}
	pthread_mutex_unlock(&mutex);

	size_t end=(i+cnt)%dstBlocksCnt;
	for (; i!=end; i=(i+1)%dstBlocksCnt) {
		for (size_t j=0; j<dstBlockLen; j++)
			*(ptr++)=dstBuffer(i,j);
	}

	pthread_mutex_lock(&mutex);
	dstR=i;
	pthread_cond_signal(&fftDstCond);
	pthread_mutex_unlock(&mutex);
	return cnt;
}

bool fftEof() {
	bool ret;
	pthread_mutex_lock(&mutex);
	ret=srcReaderEOF && !runningCnt;
	pthread_mutex_unlock(&mutex);
	return ret;
}

static void *srcReader(void *sourceFile){
	pthread_mutex_lock(&mutex);
	runningCnt++;
	FILE *f=sourceFile;
	size_t readItems=0;
	while (!feof(f)) {
		srcW=(srcW+readItems) % srcLen;
		pthread_cond_signal(&fftSrcCond);
		size_t i=0;
		while (i<workersCnt) {
			if ((srcLen+srcRFrom[i]-srcW-1)%srcLen < SRC_BLOCK_LEN) {
				pthread_cond_wait(&srcReaderCond, &mutex);
				i=0;
			} else {
				i++;
			}
		}
		pthread_mutex_unlock(&mutex);

		if (srcLen-srcW >= SRC_BLOCK_LEN)
			readItems=fread(srcBuffer+srcW, sizeof(float), SRC_BLOCK_LEN, f);
		else {
			readItems=fread(srcBuffer+srcW, sizeof(float), srcLen-srcW, f);
			if (readItems==srcLen-srcW)
				readItems+=fread(srcBuffer, sizeof(float), SRC_BLOCK_LEN-readItems, f);
		}

		pthread_mutex_lock(&mutex);
	}
	srcReaderEOF=true;
	pthread_cond_broadcast(&fftSrcCond);
	runningCnt--;
	pthread_mutex_unlock(&mutex);
	return NULL;
}

static void *fftWorker(void *info) {
	pthread_mutex_lock(&mutex);
	runningCnt++;
	pthread_mutex_unlock(&mutex);
	size_t id=((struct workerInfo*)info)->id;
	size_t *srcRFromMy=srcRFrom+id;
	size_t *dstWMy=dstW+id;
	float complex *vector=malloc(sizeof(float complex)*fftBlockLen);
	float complex *omega=malloc(sizeof(float complex)*fftBlockLenLog2);
	float maxVolume, maxAmp;
	bool wait;
	for (size_t i=0, k=1; i<fftBlockLenLog2; i++, k*=2)
		omega[i]=cexpf(I*M_PI/k);
	while (true) {
		pthread_mutex_lock(&mutex);
		do {
			wait=false;
			if (*srcRFromMy!=srcRFree) {
				*srcRFromMy=srcRFree;
				pthread_cond_signal(&srcReaderCond);
			}
			if (*dstWMy!=dstWFree) {
				*dstWMy=dstWFree;
			}
			if (((srcLen+srcW-srcRFree)%srcLen < fftBlockLen) && !srcReaderEOF) {
				wait=true;
				pthread_cond_wait(&fftSrcCond, &mutex);
				continue;
			}
			if ((dstBlocksCnt+dstR-dstWFree-1)%dstBlocksCnt < 1) {
				wait=true;
				pthread_cond_wait(&fftDstCond, &mutex);
				continue;
			}
		} while (wait);
		if ((srcLen+srcW-srcRFree)%srcLen < fftBlockLen) {
			pthread_mutex_unlock(&mutex);
			break;
		}
		srcRFree=(srcRFree+fftShiftPerBlock)%srcLen;
		dstWFree=(dstWFree+1) % dstBlocksCnt;
		pthread_mutex_unlock(&mutex);

		maxVolume=0;
		size_t i,j,k=0;
		float complex om, x, a, b;
		while (k<fftBlockLen) {
#define load(i) \
			j=cache[i].reversedIndex; \
			vector[i]=srcBuffer[(*srcRFromMy+j)%srcLen]*cache[i].windowValue; \
			if (normalizeAmps && (maxVolume<cabs(vector[i]))) \
				maxVolume=cabs(vector[i]);

#define process(mask, maskI) \
				om=omega[maskI]; \
				x=1; \
				for (j=k-(mask), i=j-(mask); j<k; i++, j++) { \
					a=vector[i]; \
					b=vector[j]; \
					vector[i]=a+x*b; \
					vector[j]=a-x*b; \
					x*=om; \
				}

			load(k); k++;
			load(k); k++;
			process(1,0);

			if (!(k&2)) {
				process(2,1);
				if (!(k&4)) {
					process(4,2);
					if (!(k&8)) {
						process(8,3);
						if (!(k&16)) {
							process(16,4);
							if (!(k&32)) {
								process(32,5);
								if (!(k&64)) {
									process(64,6);
									if (!(k&128)) {
										process(128,7);
										if (!(k&256)) {
											process(256,8);
											if (!(k&512)) {
												process(512,9);
												for (size_t mask=1024, maskI=10; !(k&mask); mask<<=1, maskI++) {
													process(mask,maskI);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
#undef load
#undef process
		}
		
		float multiplier;
		if (normalizeAmps) {
			maxAmp=FLT_MIN;
			for (size_t i=0; i<fftBlockLen; i++) {
				float value=cabs(vector[i]);
				if (maxAmp<value)
					maxAmp=value;
			}
			multiplier=maxVolume/maxAmp;
		} else {
			multiplier=1.0f/fftBlockLen;
		}

		for (size_t i=0; i<dstBlockLen; i++)
			dstBuffer(*dstWMy,i)=cabs(vector[fftOutFrom+i])*multiplier;
	}
	pthread_mutex_lock(&mutex);
	runningCnt--;
	pthread_mutex_unlock(&mutex);
	return NULL;
}
