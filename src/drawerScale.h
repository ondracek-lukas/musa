// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef DRAWER_SCALE_H
#define DRAWER_SCALE_H

#include <stdbool.h>

extern double dsColumnToPlayerPosMultiplier; // to be initialized from drawer

extern bool dsTones;
extern double dsMinFreq, dsMaxFreq, dsAnchoredFreq;

static inline int dsColumnToPlayerPos(int column) {
	return column * dsColumnToPlayerPosMultiplier;
}
static inline int dsPlayerPosToColumn(int playerPos) {
	return playerPos / dsColumnToPlayerPosMultiplier;
}

#endif
