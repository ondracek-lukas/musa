// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef DRAWER_H
#define DRAWER_H

extern struct taskInfo drawerMainTask;
extern struct taskInfo drawerConsoleTask;

extern double drawerVisibleBegin;
extern double drawerVisibleEnd;

extern void drawerInit();
extern void drawerReset();

#endif
