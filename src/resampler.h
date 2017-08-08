// MusA  Copyright (C) 2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef RESAMPLER_H
#define RESAMPLER_H

#include <stdbool.h>

extern struct taskInfo rsTask;

int rsCnt;
double *rsRates;
struct streamBuffer **rsBuffers;

extern void rsReset();
extern int rsContainsCnt(double posSec, int radius);
extern bool rsRead(int index, double posSec, int radius, float *outBuffer);


#endif
