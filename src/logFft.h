// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef LOG_FFT_H
#define LOG_FFT_H

#include <stdbool.h>
#include "fft.h"

typedef bool (logFftDataExchangeCallbackT)(
		void **info,
		float *resultData,
		size_t neighbourhoodNeeded,
		void (*loadInput)(float *data, float *dataEnd)); // dataEnd is exlusive

extern void logFftInit();

// logFftStart can be called only after logFftIsRunning returns false
extern void logFftStart(
		float minFreqMult, // min frequency in multiples of sample rate
		float maxFreqMult, // max frequency in multiples of sample rate
		int outputLen,
		logFftDataExchangeCallbackT *dataExchangeCallback);
static inline bool logFftIsRunning() {
	return fftIsRunning();
}
static inline void logFftStop() {
	fftStop();
}

static inline void logFftResume() {
	fftResume();
}


#endif
