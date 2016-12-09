// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef DRAWER_MUSIC_VISUALISER_H
#define DRAWER_MUSIC_VISUALISER_H

#include "logFft.h"

extern void dmvInit();
extern void dmvCreatePreview(int column);
extern void dmvResize(
		size_t columnLen,
		size_t visibleBefore,
		size_t visibleAfter,
		double minFreq,
		double maxFreq,
		double a1Freq);
extern void dmvRefresh();

#endif
