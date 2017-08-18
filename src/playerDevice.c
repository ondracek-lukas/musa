// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include "player.h"
#include "playerDevice.h"
#include "util.h"
#include "taskManager.h"
#include "consoleOut.h"
#include "messages.h"
#include "streamBuffer.h"

#include <portaudio.h>


static __attribute__((constructor)) void init() {
	if (Pa_Initialize() != paNoError) {
		utilExitErr("Cannot initialize PortAudio.");
	}
}

static __attribute__((destructor)) void fin() {
	Pa_Terminate();
}

PaStream *paStream = NULL;
static int paOutputStreamCallback(
		const void *input, void *output, unsigned long count,
		const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags,
		void *userData) {
	if (sbRead(&playerBuffer, playerPos, playerPos+count, output)) {
		playerPos += count;
		playerPosSec = ((double)playerPos) / playerSampleRate;
		tmResume();
		return paContinue;
	} else {
		if (!sbStreamContainsEnd(&playerBuffer, playerPos)) {
			msgSend_pause();
		}
		return paAbort;
	}
}
static int paInputStreamCallback(
		const void *input, void *output, unsigned long count,
		const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags,
		void *userData) {
	sbPreAppend(&playerBuffer, playerBuffer.end + count);
	sbWrite(&playerBuffer, playerBuffer.end, playerBuffer.end+count, input);
	sbPostAppend(&playerBuffer, playerBuffer.end + count);
	playerPos = playerBuffer.end;
	playerPosSec = ((double)playerPos) / playerSampleRate;
	playerDuration = playerPosSec;
	tmResume();
	return paContinue;
}




#define ERR(...) {consolePrintErrMsg(__VA_ARGS__); playerDeviceClose(); return false; }
bool playerDeviceOutputOpen() {
	{
		PaError err = Pa_OpenDefaultStream( &paStream,
				0, 1, paFloat32, playerSampleRate, playerSampleRate*3/100, //paFramesPerBufferUnspecified,
				paOutputStreamCallback, NULL);
		if (err != paNoError) {
			ERR("Cannot open sound card output.");
		}
	}

	playerPos=0;
	return true;
}

bool playerDeviceInputOpen(double sampleRate) {
	playerSampleRate = sampleRate;

	PaError err = Pa_OpenDefaultStream( &paStream,
			1, 0, paFloat32, playerSampleRate, playerSampleRate*3/100, //paFramesPerBufferUnspecified,
			paInputStreamCallback, NULL);

	if (err != paNoError) {
		ERR("Cannot open sound card input.");
	}

	sbReset(&playerBuffer, 0, 0, -1);

	playerDuration = -1;
	playerPos=0;
	return true;
}
#undef ERR

void playerDeviceClose() {
	playerDeviceStop();
	if (paStream) {
		Pa_CloseStream(paStream);
		paStream = NULL;
	}
}

void playerDeviceStart() {
	if (paStream) {
		if (playerPlaying) {
			if (Pa_IsStreamActive(paStream)) {
				return;
			}
			Pa_StopStream(paStream);
		} else {
			playerPlaying = true;
		}
		if (!sbStreamContainsEnd(&playerBuffer, playerPos)) {
			playerPos = 0;
			playerPosSec = 0;
		}
		Pa_StartStream(paStream);
	}
}

void playerDeviceStop() {
	if (paStream) {
		if (playerPlaying) {
			Pa_StopStream(paStream);
			playerPlaying = false;
		}
	}
}

