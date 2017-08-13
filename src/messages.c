// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include "messages.h"
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "taskManager.h"

struct msgType {
	enum msgDataType *argTypes;   // NULL terminated
	struct taskInfo **pauseTasks; // NULL terminated
	void (*callback)(void *packedArgs);
	void *option;
};

struct msgStack {
	struct msgType *type;
	void *packedData;
	struct msgStack *next;
} *messages=NULL, *pending=NULL;

struct taskInfo msgTask = TM_TASK_INITIALIZER(true, true);

static bool taskFunc();
static void __attribute__((constructor)) init() {
	tmTaskRegister(&msgTask, taskFunc, -1);
}

void msgSend(struct msgType *type, void **args) {
	void *packedArgs = msgPackArgs(type->argTypes, args);
	struct msgStack *item = utilMalloc(sizeof(struct msgStack));
	item->type = type;
	item->packedData = packedArgs;
	for (struct taskInfo **task = type->pauseTasks; *task; task++) {
		tmTaskPause(*task);
	}
	do {
		item->next = messages;
	} while (!__sync_bool_compare_and_swap(&messages, item->next, item));
	tmResume();
}

static bool taskFunc() {
	bool ret = false;
	while (pending || messages) {
		if (pending) {
			while (pending) {
				struct msgStack *item = pending;
				for (struct taskInfo **task = item->type->pauseTasks; *task; task++) {
					if (tmTaskRunning(*task)) return ret;
				}
				item->type->callback(item->packedData);
				for (struct taskInfo **task = item->type->pauseTasks; *task; task++) {
					tmTaskResume(*task);
				}
				msgFreeArgs(item->packedData);
				pending = item->next;
				free(item);
				ret = true;
			}
		}
		// assert: pending==NULL
		if (messages) {
			struct msgStack *item;
			do {
				item = messages;
			} while (!__sync_bool_compare_and_swap(&messages, item, NULL));
			while (item) {
				struct msgStack *item2 = item->next;
				item->next = pending;
				pending = item;
				item = item2;
			}
			ret = true;
		}
	}
	return ret;
}


void *msgPackArgs(enum msgDataType *types, void **argsArr) {
	size_t size = 0;
	void **arg = argsArr;
	for (enum msgDataType *t = types; *t; t++, arg++) {
		switch (*t) {
			case MSG_BOOL:   size += sizeof(bool);   break;
			case MSG_INT:    size += sizeof(int);    break;
			case MSG_FLOAT:  size += sizeof(float);  break;
			case MSG_DOUBLE: size += sizeof(double); break;
			case MSG_STRING:
			case MSG_PATH:   size += strlen(*(char**)*arg)+1; break;
			default: utilExitErr("Unknown data type to be packed.");
		}
	}
	char *packed = utilMalloc(size);
	char *p = packed;
	arg = argsArr;
	for (enum msgDataType *t = types; *t; t++, arg++) {
		switch (*t) {
			case MSG_BOOL:
				*(bool *) p = *(bool *)*arg;
				p += sizeof(bool);
				break;
			case MSG_INT:
				*(int *) p = *(int *)*arg;
				p += sizeof(int);
				break;
			case MSG_FLOAT:
				*(float *) p = *(float *)*arg;
				p += sizeof(float);
				break;
			case MSG_DOUBLE:
				*(double *) p = *(double *)*arg;
				p += sizeof(double);
				break;
			case MSG_STRING:
			case MSG_PATH:
				strcpy(p, *(char **)*arg);
				p += strlen(*(char **)*arg)+1;
				break;
			default: utilExitErr("Unknown data type to be packed.");
		}
	}
	return packed;
}

void msgUnpackArgs(enum msgDataType *types, void **argsArr, void *packedArgs) {
	char *p = packedArgs;
	void **arg = argsArr;
	for (enum msgDataType *t = types; *t; t++, arg++) {
		switch (*t) {
			case MSG_BOOL:
				*(bool *)*arg = *(bool *) p;
				p += sizeof(bool);
				break;
			case MSG_INT:
				*(int *)*arg = *(int *) p;
				p += sizeof(int);
				break;
			case MSG_FLOAT:
				*(float *)*arg = *(float *) p;
				p += sizeof(float);
				break;
			case MSG_DOUBLE:
				*(double *)*arg = *(double *) p;
				p += sizeof(double);
				break;
			case MSG_PATH:
			case MSG_STRING:
				*(char **)*arg = p;
				p += strlen(p)+1;
				break;
			default: utilExitErr("Unknown data type to be unpacked.");
		}
	}
}

void msgFreeArgs(void *packedArgs) {
	free(packedArgs);
}

#include "messagesList.gen.c"
