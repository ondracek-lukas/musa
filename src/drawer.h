// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef DRAWER_H
#define DRAWER_H

#define DRAWER_FRAME_DELAY 10   // ms

extern struct taskInfo drawerMainTask;
extern struct taskInfo drawerConsoleTask;

extern void drawerInit(
	double columnsPerSecond, double unplayedPerc);

#endif
