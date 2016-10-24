// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef DRAWER_H
#define DRAWER_H

#define DRAWER_FRAME_DELAY 10   // ms

extern void drawerInit(
	double columnsPerSecond, double centeredColumnRatio,
	double minFrequency, double maxFrequency, double anchoredFrequency,
	bool tones, bool hideScaleLines, bool showKeyboard, bool coloredOvertones);

#endif
