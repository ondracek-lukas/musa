// MusA               Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include "util.h"
#include <string.h>
#include <stdarg.h>
#include "taskManager.h"

struct taskInfo consoleStatusTask = TM_TASK_INITIALIZER(true, true);

static char *status  = NULL;
static int statusLen;

static __attribute__((constructor)) void init() {
	utilStrRealloc(&status, 0, 1);
	status[0]='\0';
}
static __attribute__((format(printf,1,2))) void append(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);
	utilStrRealloc(&status, 0, statusLen+len+1);
	va_start(args, fmt);
	statusLen += vsnprintf(status+statusLen, len+1, fmt, args);
	va_end(args);
}

#include "player.h"
#include "messages.h"
#include "drawerScale.h"
#include <math.h>


char *consoleStatusGet() {
	if (tmTaskEnter(&consoleStatusTask)) {
		statusLen = 0;

		{ // elapsed time
			int s = playerPos/playerSampleRate;
			int m = s/60; s -= m*60;
			int h = m/60; m -= h*60;
			append(" [%02d:%02d:%02d]", h, m, s);
		}

		{ // tone range
			char name[10];
			dsGetToneName(name, ceil(dsFreqToTone(msgOption_minFreq)));
			append(" [%s-", name);
			dsGetToneName(name, floor(dsFreqToTone(msgOption_maxFreq)));
			append("%s]", name);
		}
		
		{ // gain
			double signalToNoise = msgOption_signalToNoise;
			if (msgOption_dynamicGain) {
				append(" [-, %0.1lf dB]", signalToNoise);
			} else {
				double gain = msgOption_gain;
				append(" [%0.1lf dB, %0.1lf dB]", gain, signalToNoise);
			}
			
		}

		tmTaskLeave(&consoleStatusTask);
	}

	return status;
}
