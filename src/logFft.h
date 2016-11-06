// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef LOG_FFT_H
#define LOG_FFT_H

#include <stdbool.h>
#include "fft.h"

typedef void (logFftLoadInputT)(
		unsigned char precision,
		float *data,
		float *dataEnd); // exclusive

typedef bool (logFftDataExchangeCallbackT)(
		void **info,
		float *resultData,
		logFftLoadInputT *loadInput);

extern void logFftInit();

// logFftStart can be called only after logFftIsRunning returns false
extern void logFftStart(
		float minFreqMult, // min frequency in multiples of sample rate
		float maxFreqMult, // max frequency in multiples of sample rate
		int outputLen,
		logFftDataExchangeCallbackT *dataExchangeCallback,
		unsigned char *minPrecision, unsigned char *maxPrecision);
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
