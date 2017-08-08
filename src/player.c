// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include "player.h"
#include "messages.h"

static pthread_t thread;

static void *fdReader(void *sourceFile);
extern void playerUseFDQuiet(FILE *fd, float sampleRate) {
	sbReset(&playerBuffer, 0, 0, -1);
	playerPos=0;
	playerSampleRate=sampleRate;
	playerRunning=true;
	pthread_create(&thread, NULL, fdReader, fd);
	msgSend_newSource();
}

static void *fdReader(void *sourceFile){ // TODO rewrite not to store zeros
	FILE *fd=sourceFile;
	while (!feof(fd) && !ferror(fd)) {
		size_t blockSize=PLAYER_BLOCK_SIZE;
		if (sbWrapBetween(&playerBuffer, playerBuffer.end, playerBuffer.end+blockSize)) {
			blockSize = PLAYER_BUFFER_SIZE - sbPosToIndex(&playerBuffer, playerBuffer.end);
		}
		sbPreAppend(&playerBuffer, playerBuffer.end+blockSize);
		sbPostAppend(&playerBuffer, playerBuffer.end +
			fread(
				playerBuffer.data + sbPosToIndex(&playerBuffer, playerBuffer.end),
				sizeof(float), blockSize, fd));
		playerPos = playerBuffer.end;
	}
	playerRunning=false;
	return NULL;
}
