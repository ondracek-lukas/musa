// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <float.h>

#include "fft.h"

#define SRC_BLOCK_LEN 256

static size_t fftBlockLen = 0, fftBlockLenLog2;
static char *(*exchangeCallback)(void **info) = NULL;
static void (*restartCallback)() = NULL;

#define fftWindowGenHamming(x,k) ((k)-(1-(k))*cos(2*M_PI*(x)/fftBlockLen)) // Generalized Hamming window
#define fftWindowHann(x)         fftWindowGenHamming((x),0.5)              // Hann window
#define fftWindowHamming(x)      fftWindowGenHamming((x),0.54)             // Hamming window
#define fftWindowRect(x)         1                                         // Rectangular window
#define fftWindowTrgl(x)         ((float)((x)>fftBlockLen/2?fftBlockLen-(x):(x))*2/fftBlockLen) // Triangular window

#define fftWindow fftWindowHann
static struct cache {
	unsigned reversedIndex;
	float windowValue;
} *cache = NULL;
float complex *omega = NULL;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

unsigned workersCnt;
static struct workerInfo {
	unsigned id;
	pthread_t threadID;
	void *info;
} *workers;

static void *fftWorker(void *null);

static unsigned runningCnt=0;
static unsigned waitingCnt=0;

static inline size_t reverseLowerBits(size_t value, size_t cnt) {
	value = ((value & 0b1010101010101010) >> 1) | ((value & 0b0101010101010101) << 1);
	value = ((value & 0b1100110011001100) >> 2) | ((value & 0b0011001100110011) << 2);
	value = ((value & 0b1111000011110000) >> 4) | ((value & 0b0000111100001111) << 4);
	value = ((value & 0b1111111100000000) >> 8) | ((value & 0b0000000011111111) << 8);
	return value >> (16-cnt);
}

static void cacheOmegaInit() {
	free(cache);
	cache=malloc(sizeof(struct cache)*fftBlockLen);
	for (size_t i=0; i<fftBlockLen; i++) {
		size_t j=reverseLowerBits(i,fftBlockLenLog2);
		cache[i].reversedIndex=j;
		cache[i].windowValue=fftWindow(j);
	}
	free(omega);
	omega=malloc(sizeof(float complex)*fftBlockLenLog2);
	for (size_t i=0, k=1; i<fftBlockLenLog2; i++, k*=2)
		omega[i]=cexpf(I*M_PI/k);
}

extern void fftInit(unsigned threads) {
	workersCnt=threads;
	workers=malloc(sizeof(struct workerInfo)*workersCnt);
	for (size_t i=0; i<workersCnt; i++) {
		workers[i].id=i;
		workers[i].info=NULL;
		pthread_create(&workers[i].threadID, NULL, fftWorker, workers+i);
	}
}

static bool restartInvoked = false;
static size_t newBlockLenLog2;
static char *(*newExchangeCallback)(void **info);
extern void fftRestart(size_t blockLenLog2, void (*oneTimeCallback)(), char *(*dataExchangeCallback)(void **info)) {
	pthread_mutex_lock(&mutex);
	restartInvoked = true;
	if (sleepingCnt >= runningCnt) {
		pthread_cond_signal(&cond);
	}
	newBlockLenLog2 = blockLenLog2;
	restartCallback = oneTimeCallback;
	newExchangeCallback = dataExchangeCallback;
	pthread_mutex_unlock(&mutex);
}
static void onRestart() {  // to be called with mutex locked in the only non-waiting thread
	fftBlockLenLog2 = blockLenLog2;
	fftBlockLen = 1<<blockLenLog2;
	exchangeCallback = newExchangeCallback;
	cacheOmegaInit();
	restartCallback();
	restartInvoked = false;
	pthread_cond_broadcast(&cond);
}

extern void fftResume() {
	pthread_cond_broadcast(&cond);
}


static void *fftWorker(void *info) {
	pthread_mutex_lock(&mutex);
	runningCnt++;
	if (!fftBlockLen) {
		waitingCnt++;
		if (!restartInvoked) {
			pthread_cond_wait(&cond, &mutex);
		}
		if (restartInvoked) {
			onRestart();
		}
		waitingCnt--;
	}
	pthread_mutex_unlock(&mutex);


	char *buffer = NULL;
	size_t vectorLen=0;
	float complex *vector = NULL;
	float maxVolume, maxAmp;

	while (true) {
		while (true) {
			buffer = exchangeCallback(&((struct workerInfo*)info)->info);
			if (buffer) {
				__sync_synchronize();
				if (waitingCnt) {
					fftResume();
				}
				break;
			} else {
				pthread_mutex_lock(&mutex);
				waitingCnt++;
				if (restartInvoked && (waitingCnt >= runningCnt)) {
					onRestart();
				} else {
					pthread_cond_wait(&cond, &mutex);
				}
				waitingCnt--;
				pthread_mutex_unlock(&mutex);
			}
		}
		if (vectorLen != fftBlockLen) {
			vectorLen = fftBlockLen;
			free(vector);
			vector = malloc(sizeof(float complex)*vectorLen);
		}

		maxVolume=0;
		size_t i,j,k=0;
		float complex om, x, a, b;
		while (k<fftBlockLen) {
#define load(i) \
			j=cache[i].reversedIndex; \
			vector[i] = buffer[j] * cache[i].windowValue; \
			if (maxVolume<cabs(vector[i])) \
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
		maxAmp=FLT_MIN;
		for (size_t i=0; i<fftBlockLen; i++) {
			float value=cabs(vector[i]);
			if (maxAmp<value)
				maxAmp=value;
		}
		multiplier=maxVolume/maxAmp;

		for (size_t i=0; i<=fftBlockLen; i++)  // copy the first half only
			buffer[i]=cabs(vector[i])*multiplier;
		buffer = NULL;
	}
	pthread_mutex_lock(&mutex);
	runningCnt--;
	pthread_mutex_unlock(&mutex);
	return NULL;
}
