// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef DRAWER_SCALE_H
#define DRAWER_SCALE_H

#include <stdbool.h>


// --- TIME SCALE ---

extern double dsColumnToPlayerPosMultiplier;

static inline void dsSetTimeScale(double sampleRate, double columnsPerSecond) {
	dsColumnToPlayerPosMultiplier = sampleRate / columnsPerSecond;
}

static inline int dsColumnToPlayerPos(int column) {
	return column * dsColumnToPlayerPosMultiplier;
}

static inline int dsPlayerPosToColumn(int playerPos) {
	return playerPos / dsColumnToPlayerPosMultiplier;
}


// --- TONE SCALE ---

#define DS_OVERTONES_CNT 16

extern double dsMinFreq, dsMaxFreq, dsA1Freq;
extern double dsSemitoneOffset, dsA1Index;
extern struct dsOvertonesS {
	int offset;          // lower whole part of the offset
	float offsetFract;   // the fractional part
} dsOvertones[DS_OVERTONES_CNT];

extern void dsSetToneScale(double minFreq, double maxFreq, double a1Freq);

#endif
