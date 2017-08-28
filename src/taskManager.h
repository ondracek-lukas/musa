// MusA  Copyright (C) 2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

// taskManager module repeatedly executes registered tasks in parallel
// and provides lock-free mechanism to suspend task execution
// in order to safely modify their data structures.


// Task is defined by its taskInfo structure
// and any number of registered functions with assigned priorities.
// It can be serial,
// which means, that it can be executed by at most one thread at a time
// (but the threads can rotate).
// It can be temporarily paused or made (in)active.

// Registered functions should perform their work and return true,
// or return false if there is currently no work;
// they are executed only if there is no work with lower priority number.
// In other words, functions are being executed in ascending priority number
// as long as they are returning false; then the cycle starts from the beginning.

// If there is no work at all, threads are suspended
// and should be awaken by tmResume() if the situation changes.

// Tasks can be also executed as foreign,
// which means that some code of a foreign thread
// is considered to be part of the task
// and should be executed only if
//    * task is active,
//    * task is not paused, and
//    * task is not serial or not running at the time.

// The only lock and conditional variable are used for suspending the threads;
// there are no locks in use if there is enough work.

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <stdbool.h>


// Following two defines should be commented in release.

// Print statistics on stop/exit.
#define TM_PROFILER

// Print debug messages.
//#define TM_LOG



// It is safe to change active/serial flags any time,
// but it is recommended to change them when task is paused,
// to be sure it takes effect immediately.

struct taskInfo {
	bool active;
	bool serial;
	int running;     // read-only
	int paused;      // read-only
};
#define TM_TASK_INITIALIZER(active,serial) ((struct taskInfo){(active), (serial), 0, 0})



// Task registering should be performed before first tmResume call,
// it is NOT thread-safe.
void tmTaskRegisterName(struct taskInfo *task, bool(*func)(), int priority, const char *name);
#define tmTaskRegister(task, func, priority) tmTaskRegisterName(task, func, priority, #task)


// Example of task pausing from one thread:
//   tmTaskPause(&myTask);
//   while (tmTaskRunning(&myTask)) do another work;
//   ... // task is paused here
//   tmTaskResume(&myTask);
//
// Tasks can be paused multiple times and they should be resumed the same number of times.
// Calling tmTaskRunning makes sense only if tmTaskPause is in action
// (e.g. it was called from the same thread).

#define tmTaskPause(task)   __sync_fetch_and_add(&(task)->paused, 1)
#define tmTaskRunning(task) (__sync_synchronize(), (task)->running)
#define tmTaskResume(task)  __sync_fetch_and_sub(&(task)->paused, 1)


// Example of foreign task execution:
//   if(tmTaskEnter(&myTask)) {
//      ... // do the work
//      tmTaskLeave(&myTask);
//   }

inline void tmTaskLeave(struct taskInfo *task) { // to be used for foreign task execution only
	__sync_fetch_and_sub(&task->running, 1);
}
inline bool tmTaskEnter(struct taskInfo *task) { // to be used for foreign task execution only
	if (!task->active || task->paused) return false;
	if (task->serial) {
		if (!__sync_bool_compare_and_swap(&task->running, 0, 1)) return false;
	} else {
		__sync_fetch_and_add(&task->running, 1);
	}
	__sync_synchronize();
	if (task->paused) {
		tmTaskLeave(task);
		return false;
	}
	return true;
}


// tmResume should be called after all functions are registered
// to resume threads for the first time,
// and then every time new work might be created.
// It is unnecessary to call it from registered functions returning true.
void tmResume();

// tmStop terminates the threads,
// any further use of tmManager is erroneous.
// It is automatically called on exit,
// so it is not necessary to call it at all.
void tmStop();

#endif
