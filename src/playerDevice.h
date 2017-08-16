// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef PLAYER_DEVICE_H
#define PLAYER_DEVICE_H

#include <stdbool.h>

bool playerDeviceOutputOpen();
bool playerDeviceInputOpen(double sampleRate);
void playerDeviceClose();
void playerDeviceStart();
void playerDeviceStop();

#endif
