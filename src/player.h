// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>
#include "streamBuffer.h"

#define PLAYER_BUFFER_SIZE STREAM_BUFFER_SIZE
#define PLAYER_BLOCK_SIZE (2<<8)

struct streamBuffer playerBuffer;

int playerPos;      // current position in the stream
bool playerRunning;  // end of stream has not been reached
double playerSampleRate;

extern void playerUseFDQuiet(FILE *fd, float sampleRate);

bool playerPlaying;
extern void playerPlay();
extern void playerPause();

#endif
