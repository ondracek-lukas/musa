// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include "player.h"

static pthread_t thread;

static void *fdReader(void *sourceFile);
extern void playerUseFDQuiet(FILE *fd, float freqRate) {
	for (int i = 0; i < PLAYER_BUFFER_SIZE; i++) {
		playerBuffer.data[i] = 0;
	}
	playerBuffer.begin=-PLAYER_BUFFER_SIZE;
	playerBuffer.end=0;
	playerPos=0;
	playerFreqRate=freqRate;
	playerRunning=true;
	pthread_create(&thread, NULL, fdReader, fd);
}

static void *fdReader(void *sourceFile){
	FILE *fd=sourceFile;
	while (!feof(fd) && !ferror(fd)) {
		size_t blockSize=PLAYER_BLOCK_SIZE;
		if (playerBufferWrapBetween(playerBuffer.end, playerBuffer.end+blockSize)) {
		//if (playerPosToIndex(playerBuffer.end) + blockSize > PLAYER_BUFFER_SIZE) {
			blockSize = PLAYER_BUFFER_SIZE - playerPosToIndex(playerBuffer.end);
		}
		playerBuffer.begin += blockSize;
		__sync_synchronize();
		playerBuffer.end += fread(
			playerBuffer.data + playerPosToIndex(playerBuffer.end),
			sizeof(float), blockSize, fd);
		__sync_synchronize();
		playerBuffer.begin = playerBuffer.end - PLAYER_BUFFER_SIZE;
		playerPos = playerBuffer.end;
	}
	playerRunning=false;
	return NULL;
}
