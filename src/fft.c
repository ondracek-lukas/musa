// MusA  Copyright (C) 2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <float.h>
#include "util.h"

#include "fft.h"

#define SRC_BLOCK_LEN 256

static size_t blockLen = 0, blockLenLog2;

#define fftWindowGenHamming(x,k) ((k)-(1-(k))*cos(2*M_PI*(x)/blockLen)) // Generalized Hamming window
#define fftWindowHann(x)         fftWindowGenHamming((x),0.5)              // Hann window
#define fftWindowHamming(x)      fftWindowGenHamming((x),0.54)             // Hamming window
#define fftWindowRect(x)         1                                         // Rectangular window
#define fftWindowTrgl(x)         ((float)((x)>blockLen/2?blockLen-(x):(x))*2/blockLen) // Triangular window

#define fftWindow fftWindowHann
static struct cache {
	unsigned reversedIndex;
	float windowValue;
} *cache = NULL;
float complex *omega = NULL;


static inline size_t reverseLowerBits(size_t value, size_t cnt) {
	value = ((value & 0b1010101010101010) >> 1) | ((value & 0b0101010101010101) << 1);
	value = ((value & 0b1100110011001100) >> 2) | ((value & 0b0011001100110011) << 2);
	value = ((value & 0b1111000011110000) >> 4) | ((value & 0b0000111100001111) << 4);
	value = ((value & 0b1111111100000000) >> 8) | ((value & 0b0000000011111111) << 8);
	return value >> (16-cnt);
}

static void cacheOmegaInit() {
	free(cache);
	cache=utilMalloc(sizeof(struct cache)*blockLen);
	for (size_t i=0; i<blockLen; i++) {
		size_t j=reverseLowerBits(i,blockLenLog2);
		cache[i].reversedIndex=j;
		cache[i].windowValue=fftWindow(j);
	}
	free(omega);
	omega=utilMalloc(sizeof(float complex)*blockLenLog2);
	for (size_t i=0, k=1; i<blockLenLog2; i++, k*=2)
		omega[i]=cexpf(I*M_PI/k);
}

extern void fftReset(size_t newBlockLenLog2) {
	blockLenLog2 = newBlockLenLog2;
	blockLen = 1<<blockLenLog2;
	cacheOmegaInit();
}



void fftProcess(float *buffer) {
	float complex *vector = utilMalloc(sizeof(float complex)*blockLen); // TODO make it part of the reentrant data

	size_t i,j,k=0;
	float complex om, x, a, b;
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

	free(vector);
}
