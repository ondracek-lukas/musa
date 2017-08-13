// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include "player.h"
#include "messages.h"
#include "taskManager.h"

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

bool playerPlaying = false;

static void *fdReader(void *sourceFile){
	FILE *fd=sourceFile;
	playerPlaying = true;
	while (!feof(fd) && !ferror(fd)) {
		size_t len=PLAYER_BLOCK_SIZE;
		if (sbWrapBetween(&playerBuffer, playerBuffer.end, playerBuffer.end+len)) {
			len = PLAYER_BUFFER_SIZE - sbPosToIndex(&playerBuffer, playerBuffer.end);
		}
		sbPreAppend(&playerBuffer, playerBuffer.end+len);
		len = fread(
				playerBuffer.data + sbPosToIndex(&playerBuffer, playerBuffer.end),
				sizeof(float), len, fd);
		if (playerPlaying) {
			sbPostAppend(&playerBuffer, playerBuffer.end + len);
			tmResume();
		}
		playerPos = playerBuffer.end;
	}
	playerRunning=false;
	playerPlaying = false;
	return NULL;
}


extern void playerPlay() {
	playerPlaying = true;
}
extern void playerPause() {
	playerPlaying = false;
}
