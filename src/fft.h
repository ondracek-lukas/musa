// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

extern void fftInit(unsigned threads);
extern void fftRestart(size_t blockLenPow2, void (*oneTimeCallback)(), char *(*dataExchangeCallback)(void **info));
extern void fftResume();
//extern void fftDestroy();
