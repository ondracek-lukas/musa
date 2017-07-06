// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef LOG_FFT_H
#define LOG_FFT_H

#include <stdbool.h>
#include "fft.h"

extern void logFftReset(
		float minFreqMult, // min frequency in multiples of sample rate
		float maxFreqMult, // max frequency in multiples of sample rate
		int outputLen,
		unsigned char *minPrecision, unsigned char *maxPrecision);

extern void logFftProcess(unsigned char precision, float *inputBuffer, float *outputBuffer);



#endif
