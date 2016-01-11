// Spectrograph  Copyright (C) 2015  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

extern void fftStart(FILE *source, size_t blockLenPow2, size_t shiftPerBlock, size_t outputFromIndex, size_t outputToIndex, bool normalizeAmplitudes, unsigned fftThreads);
extern size_t fftGetData(float *ptr, size_t maxBlocksCnt);
extern bool fftEof();
