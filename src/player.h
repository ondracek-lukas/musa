// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>

#define PLAYER_BUFFER_SIZE (2<<20)
#define PLAYER_BLOCK_SIZE (2<<8)

struct {
	int begin; // position in the stream of the first sample in buffer
	int end;   // position in the stream of the first sample not in buffer
	float data[PLAYER_BUFFER_SIZE];  // indexes are positions in stream mod PLAYER_BUFFER_SIZE
} playerBuffer;

#define playerPosToIndex(pos) (((pos)+PLAYER_BUFFER_SIZE)%PLAYER_BUFFER_SIZE)
#define playerBuffer(pos) playerBuffer.data[playerPosToIndex(pos)]
#define playerBufferWrapBetween(pos1, pos2) (((pos1)+PLAYER_BUFFER_SIZE)/PLAYER_BUFFER_SIZE != ((pos2)+PLAYER_BUFFER_SIZE-1)/PLAYER_BUFFER_SIZE)

int playerPos;      // current position in the stream
bool playerRunning;  // end of stream has been reached
double playerFreqRate;

extern void playerUseFDQuiet(FILE *fd, float freqRate);

#endif
