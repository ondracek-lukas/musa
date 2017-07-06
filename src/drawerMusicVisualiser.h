// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef DRAWER_MUSIC_VISUALISER_H
#define DRAWER_MUSIC_VISUALISER_H

#include "logFft.h"

extern struct taskInfo dmvTask;

extern void dmvInit();
extern void dmvCreatePreview(int column);
extern void dmvReset(void); // pause dmvTask and drawerMainTask before
extern void dmvRefresh();

#endif
