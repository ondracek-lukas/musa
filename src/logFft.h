// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef LOG_FFT_H
#define LOG_FFT_H

#include <stdbool.h>
#include "fft.h"

extern void logFftReset(
		float newMinFreq,
		float newMaxFreq,
		int newOutputLen,
		int *blockLenOut);

extern bool logFftProcess(double posSec, int blocksCnt, float *outputBuffer);



#endif
