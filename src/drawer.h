// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef DRAWER_H
#define DRAWER_H

#define DRAWER_FRAME_DELAY 10   // ms

extern void drawerInit(
	double columnsPerSecond, double unplayedPerc,
	double minFrequency, double maxFrequency, double a1Frequency);

#endif
