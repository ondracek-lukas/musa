// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

extern void logFftSetParams(
	int minPeriod,
	int maxPeriod,
	int outputLen,
	int (*callback)(int resultIndex, float *resultData)
);
extern void logFftStart();


// to be called from callback
extern void logFftLoadInput(size_t size, float *data);
extern void logFftResetInput();
