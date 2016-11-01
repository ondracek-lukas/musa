// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef FFT_H
#define FFT_H

#include <stdbool.h>
#include <stdlib.h>

extern void fftInit();
//extern void fftDestroy();

// fftStart can be called only after fftIsRunning returns false
extern void fftStart(
		size_t blockLenLog2,
		float *(*dataExchangeCallback)(void **info, bool restartNeeded));
extern bool fftIsRunning();
extern void fftStop();

extern void fftResume();

#endif
