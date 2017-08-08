// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef FFT_H
#define FFT_H

#include <stdbool.h>
#include <stdlib.h>

struct fftSpectrumContext;

extern struct fftSpectrumContext *fftCreateSpectrumContext(size_t newBlockLenLog2);
extern void fftDestroySpectrumContext(struct fftSpectrumContext *context);

extern void fftSpectrum(float *buffer, struct fftSpectrumContext *context);


struct fftFilterContext;

extern struct fftFilterContext *fftCreateFilterContext(float *impulseResponse, int impulseResponseLen, int fftBlockLenLog2);
extern void fftDestroyFilterContext(struct fftFilterContext *context);
extern void fftFilter(float *buffer, struct fftFilterContext *context);

#endif
