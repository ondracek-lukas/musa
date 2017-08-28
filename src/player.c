// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "player.h"
#include "playerFile.h"
#include "playerDevice.h"
#include "messages.h"
#include "taskManager.h"
#include "consoleOut.h"
#include "util.h"
#include "resources.gen.h"

enum playerSourceType playerSourceType = PLAYER_SOURCE_NONE;
char *playerSource = NULL;
int playerPos = 0;
double playerPosSec = 0;
double playerDuration = 0;
bool playerPlaying = false;

static __attribute__((constructor)) void init() {
	sbReset(&playerBuffer, 0, 0, 0);
	playerPos=0;
	playerSampleRate=1;
}

static void close();
void playerOpenDeviceDefault() {
	playerOpenDevice(44100);
}
void playerOpenDevice(double sampleRate) {
	close();
	if (!playerDeviceInputOpen(sampleRate)) {
		return;
	}

	char *source = utilMalloc(sizeof(char) * 13);
	strcpy(source, "input device");
	playerSource = source;
	playerSourceType = PLAYER_SOURCE_DEVICE;

	msgSend_newSource();
	playerPlay();
}

void playerOpen(char *filename) {
	close();
	filename = utilExpandPath(filename);

	if (!playerFileOpen(filename)) {
		return;
	}

	if (!playerDeviceOutputOpen()) {
		playerFileClose();
		return;
	}

	char *source = utilMalloc(sizeof(char) * strlen(filename));
	strcpy(source, filename);
	playerSource = source;
	playerSourceType = PLAYER_SOURCE_FILE;

	msgSend_newSource();
	playerPlay();
}

#define LOGO_OPTIONS \
	OPT2(dynamicGain,      false); \
	OPT2(filterOvertones,  false); \
	OPT2(forceCursorPos,   false); \
	OPT2(gain,             6); \
	OPT1(maxFreq); \
	OPT1(minFreq); \
	OPT1(outputRate); \
	OPT2(reverseDirection, false); \
	OPT2(showCursor,       true); \
	OPT2(showGrid,         false); \
	OPT2(signalToNoise,    32); \
	OPT2(swapAxes,         false)

#define OPT1(name) typeof(msgOption_##name) origOption_##name
#define OPT2(name, value) OPT1(name)
LOGO_OPTIONS;
#undef OPT1
#undef OPT2

void playerResetLogoSize(double width, double fractHeight, double fractVertPos) {
	if (width < 1) return;
	const double logoMaxFreq = 4000; // Hz
	const double logoMinFreq =  400; // Hz
	const double logoLen     =    7; // s
	msgSet_outputRate(width / logoLen);

	const double logoFreqRatio = logoMaxFreq/logoMinFreq;
	const double freqRatio = pow(logoFreqRatio, 1/fractHeight);
	const double minFreq   = logoMinFreq * sqrt(logoFreqRatio) / pow(freqRatio, fractVertPos);
	msgSet_maxFreq(minFreq * freqRatio);
	msgSet_minFreq(minFreq);
}
void playerOpenLogo() {
	close();

	if (!playerDataOpen("logo.mp3", resources_logo_mp3, sizeof(resources_logo_mp3))) {
		return;
	}

	if (!playerDeviceOutputOpen()) {
		playerFileClose();
		return;
	}

#define OPT1(name) \
	msgSetOptionEnabled_##name(false); \
	origOption_##name = msgOption_##name
#define OPT2(name, value) \
	OPT1(name); \
	msgSet_##name(value)
LOGO_OPTIONS;
#undef OPT1
#undef OPT2

	playerSourceType = PLAYER_SOURCE_LOGO;

	msgSend_newSource();
	playerPlay();
}

static void close() {
	enum playerSourceType type = playerSourceType;
	playerSourceType = PLAYER_SOURCE_NONE;
	if (playerSource) {
		char *source = playerSource;
		playerSource = NULL;
		free(source);
	}
	switch (type) {
		case PLAYER_SOURCE_LOGO:
			playerDataClose();
			playerDeviceClose();
			break;
		case PLAYER_SOURCE_FILE:
			playerFileClose();
			playerDeviceClose();
			break;
		case PLAYER_SOURCE_DEVICE:
			playerDeviceClose();
			break;
	}
	int playerPos = 0;
	double playerPosSec = 0;
	double playerDuration = 0;
	sbReset(&playerBuffer, 0, 0, 0);
	if (type == PLAYER_SOURCE_LOGO) {
#define OPT1(name) \
	msgSet_##name(origOption_##name); \
	msgSetOptionEnabled_##name(true)
#define OPT2(name, value) OPT1(name)
LOGO_OPTIONS;
#undef OPT1
#undef OPT2
	}
}


extern void playerPlay() {
	switch (playerSourceType) {
		case PLAYER_SOURCE_LOGO:
		case PLAYER_SOURCE_FILE:
		case PLAYER_SOURCE_DEVICE:
			playerDeviceStart();
			break;
	}
}

extern void playerPause() {
	switch (playerSourceType) {
		case PLAYER_SOURCE_LOGO:
		case PLAYER_SOURCE_FILE:
		case PLAYER_SOURCE_DEVICE:
			playerDeviceStop();
			break;
	}
}

extern void playerSeekAbs(double posSec) {
	switch (playerSourceType) {
		case PLAYER_SOURCE_FILE:
		case PLAYER_SOURCE_LOGO:
			{
				bool playing = playerPlaying;
				if (playing) {
					playerDeviceStop();
				}
				if (posSec > playerDuration) {
					posSec = playerDuration;
				}
				if (posSec < 0) posSec = 0;
				int pos = posSec * playerSampleRate;
				playerPos = pos;
				playerPosSec = posSec;
				if (playing) {
					playerDeviceStart();
				}
				break;
			}
		default:
			msgSend_printErr("Cannot seek.");
	}
}
extern void playerSeekRel(double sec) {
	playerSeekAbs(playerPosSec + sec);
}
