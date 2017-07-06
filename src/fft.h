// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef FFT_H
#define FFT_H

#include <stdbool.h>
#include <stdlib.h>

extern void fftReset(size_t blockLenLog2);    // TODO rename and make it reentrant
extern void fftProcess(float *buffer);

#endif
