// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>
#include "streamBuffer.h"

#define PLAYER_BUFFER_SIZE STREAM_BUFFER_SIZE

struct streamBuffer playerBuffer;

int playerPos;       // current position in the stream
double playerPosSec;
double playerDuration;
double playerSampleRate;
enum playerSourceType {
	PLAYER_SOURCE_NONE = 0,
	PLAYER_SOURCE_LOGO,
	PLAYER_SOURCE_FILE,
	PLAYER_SOURCE_DEVICE
} playerSourceType;
char *playerSource;

extern void playerOpen(char *filename);
extern void playerOpenDevice(double sampleRate);
extern void playerOpenDeviceDefault();
extern void playerOpenLogo();
extern void playerResetLogoSize(double width, double fractHeight, double fractVertPos);

bool playerPlaying;
extern void playerPlay();
extern void playerPause();
extern void playerSeekAbs(double posSec);
extern void playerSeekRel(double sec);

#endif
