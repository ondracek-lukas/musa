// MusA  Copyright (C) 2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <stdbool.h>

struct taskInfo {
	bool active;
	bool serial;
	int running;     // read-only
	// bool paused; ?
};

void tmRegister(struct taskInfo *task, bool(*func)(), int priority);
void tmResume();
void tmStop();

int *tmBarrierCreate();
void tmBarrierPlace(int *barrier);
bool tmBarrierReached(int *barrier);
void tmBarrierFree(int *barrier);

// TODO add external/foreign tasks
// TODO add pausing tasks (instead of barriers?)

#define TM_PROFILER
//#define TM_LOG

#endif
