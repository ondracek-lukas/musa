// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <float.h>
#include "mem.h"

#include "fft.h"

int log2CacheHash[37] = {0, 0, 1, 26, 2, 23, 27, 0, 3, 16, 24, 30, 28, 11, 0, 13, 4, 7, 17, 0, 25, 22, 31, 15, 29, 10, 12, 6, 0, 21, 14, 9, 5, 20, 8, 19, 18};
#define log2Cache(x) log2CacheHash[(x)%37]

static inline size_t reverseLowerBits(size_t value, size_t cnt) {
	value = ((value & 0b1010101010101010) >> 1) | ((value & 0b0101010101010101) << 1);
	value = ((value & 0b1100110011001100) >> 2) | ((value & 0b0011001100110011) << 2);
	value = ((value & 0b1111000011110000) >> 4) | ((value & 0b0000111100001111) << 4);
	value = ((value & 0b1111111100000000) >> 8) | ((value & 0b0000000011111111) << 8);
	return value >> (16-cnt);
}


// --- SPECTRUM ---

#define fftWindowGenHamming(x,blockLen,k) ((k)-(1-(k))*cos(2*M_PI*(x)/blockLen))                // Generalized Hamming window
#define fftWindowHann(x,blockLen)         fftWindowGenHamming((x),blockLen,0.5)                 // Hann window
#define fftWindowHamming(x,blockLen)      fftWindowGenHamming((x),blockLen,0.54)                // Hamming window
#define fftWindowRect(x,blockLen)         1                                                     // Rectangular window
#define fftWindowTrgl(x,blockLen)         ((float)((x)>blockLen/2?blockLen-(x):(x))*2/blockLen) // Triangular window

#define fftWindow fftWindowHann
struct cache {
	unsigned reversedIndex;
	float windowValue;
};

struct fftSpectrumContext {
	size_t blockLen, blockLenLog2;
	struct cache *cache;
	double complex *omega;
	struct memPerThread *vector;
};


struct fftSpectrumContext *fftCreateSpectrumContext(size_t newBlockLenLog2) {
	struct fftSpectrumContext *context = memMalloc(sizeof(struct fftSpectrumContext));
	context->blockLenLog2 = newBlockLenLog2;
	context->blockLen = 1<<newBlockLenLog2;
	context->cache=memMalloc(sizeof(struct cache)*context->blockLen);
	for (size_t i=0; i<context->blockLen; i++) {
		size_t j=reverseLowerBits(i,context->blockLenLog2);
		context->cache[i].reversedIndex=j;
		context->cache[i].windowValue=fftWindow(j,context->blockLen);
	}
	context->omega=memMalloc(sizeof(double complex)*context->blockLenLog2);
	for (size_t i=0, k=1; i<context->blockLenLog2; i++, k*=2)
		context->omega[i]=cexp(I*M_PI/k);
	context->vector = memPerThreadMalloc(sizeof(float complex) * context->blockLen);
	return context;
}

void fftDestroySpectrumContext(struct fftSpectrumContext *context) {
	if (!context) return;
	free(context->cache);
	free(context->omega);
	memPerThreadFree(context->vector);
	free(context);
}



void fftSpectrum(float *buffer, struct fftSpectrumContext *context) {
	size_t blockLen = context->blockLen;
	struct cache *cache = context->cache;
	double complex *omega = context->omega;
	float complex *vector = memPerThreadGet(context->vector);


	size_t i,j,k=0;
	double complex om, x, a, b;
	while (k<blockLen) {
#define load(i) \
		j=cache[i].reversedIndex; \
		vector[i] = buffer[j] * cache[i].windowValue;

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

	for (size_t i=0; i<=blockLen/2; i++)  // copy the first half only
		buffer[i]=cabs(vector[i])/blockLen;
	buffer = NULL;

}



// --- FILTERS ---


// fft returning data to be shuffled
static void fftNoShuffle(complex float *vector, size_t blockLen, double complex *omega) {

	int i,j,k=0;
	double complex om, x, a, b;
	while (k<blockLen) {

#define process(mask, maskI) \
			om=omega[maskI]; \
			x=1; \
			for (i=k, j=k+(mask); i<k+(mask); i++, j++) { \
				a=vector[i]; \
				b=vector[j]; \
				vector[i]=(a+b); \
				vector[j]=(a-b)*x; \
				x*=om; \
			}


		for (size_t mask = (~(blockLen+k-1) & (blockLen+k)) >> 1, maskI=log2Cache(mask); mask; mask>>=1, maskI--) {
			process(mask,maskI);
		}

		k+=2;

#undef process
	}
}

// fft expecting shuffled data
static void fftNoShuffleR(complex float *vector, size_t blockLen, double complex *omega) {

	size_t i,j,k=0;
	double complex om, x, a, b;
	while (k<blockLen) {

#define process(mask, maskI) \
			om=omega[maskI]; \
			x=1; \
			for (j=k-(mask), i=j-(mask); j<k; i++, j++) { \
				a=vector[i]; \
				b=vector[j]; \
				vector[i]=(a+x*b)/2; \
				vector[j]=(a-x*b)/2; \
				x*=om; \
			}

		k+=2;
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
#undef process
	}
}


struct fftFilterContext {
	size_t blockLen;
	size_t impulseResponseLen;
	double complex *omega, *omegaR;
	complex float *responseSpectrum;
	struct memPerThread *vector;
};

struct fftFilterContext *fftCreateFilterContext(float *impulseResponse, int impulseResponseLen, int fftBlockLenLog2) {

	struct fftFilterContext *context = memMalloc(sizeof(struct fftFilterContext));

	context -> blockLen = (1<<fftBlockLenLog2);
	context -> impulseResponseLen = impulseResponseLen;

	context->omega=memMalloc(sizeof(double complex)*fftBlockLenLog2);
	for (size_t i=0, k=1; i<fftBlockLenLog2; i++, k*=2)
		context->omega[i]=cexp(I*M_PI/k);

	context->omegaR=memMalloc(sizeof(double complex)*fftBlockLenLog2);
	for (size_t i=0, k=1; i<fftBlockLenLog2; i++, k*=2)
		context->omegaR[i]=cexp(-I*M_PI/k);

	context -> responseSpectrum = memCalloc(context->blockLen, sizeof(complex float));


	for (int i = 0; i < impulseResponseLen; i++) {
		context->responseSpectrum[i] = impulseResponse[i];
	}
	fftNoShuffle(context->responseSpectrum, context->blockLen, context->omega);

	context->vector = memPerThreadMalloc(sizeof(float complex) * context->blockLen);

	return context;
}
void fftDestroyFilterContext(struct fftFilterContext *context) {
	if (context) {
		free(context->omega);
		free(context->omegaR);
		free(context->responseSpectrum);
		memPerThreadFree(context->vector);
		free(context);
	}
}
void fftFilter(float *buffer, struct fftFilterContext *context) {
	int blockLen = context->blockLen;
	complex float *vector = memPerThreadGet(context->vector);
	for (int i = 0; i<blockLen; i++) {
		vector[i] = buffer[i];
	}
	fftNoShuffle(vector, context->blockLen, context->omega);
	for (int i = 0; i < blockLen; i++) {
		vector[i] *= context->responseSpectrum[i];
	}
	fftNoShuffleR(vector, context->blockLen, context->omegaR);

	for (int i = context->impulseResponseLen-1, j=0; i<blockLen; i++, j++) {
		buffer[j] = vector[i];
	}
}
