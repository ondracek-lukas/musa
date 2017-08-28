// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef PLAYER_FILE_H
#define PLAYER_FILE_H

#include <stdbool.h>

extern struct taskInfo playerFileTask;
extern struct taskInfo playerFileThreadTask;
extern bool playerFileOpen(char *filename);
extern void playerFileClose();
extern bool playerDataOpen(char *filename, const char *data, size_t size);
extern void playerDataClose();

#endif
