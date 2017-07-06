// MusA               Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file
// Geometric Figures  Copyright (C) 2015--2016  Lukáš Ondráček <ondracek.lukas@gmail.com>

#include "consoleIn.h"

#include <string.h>
#include "commandParser.h"
#include "messages.h"
#include "util.h"

static struct utilStrList *historyFirst;
static struct utilStrList *historyLast;
static struct utilStrList *historyActive;
static int historyCount;
static int historyMaxCount;
static int cmdEnd;
static int cmdBegin;
static int cursorPos;


// -- history --

static struct utilStrList *historyFirst=0;
static struct utilStrList *historyLast=0;
static struct utilStrList *historyActive=0;
static int historyCount=0;
static int historyMaxCount=20;

int consoleGetHistoryMaxCount() {
	return historyMaxCount;
}

void consoleSetHistoryMaxCount(int maxCount) {
	historyMaxCount=maxCount;
	for (; historyCount>historyMaxCount; historyCount--)
		if (historyCount>1) {
			historyFirst=historyFirst->next;
			utilStrRealloc(&historyFirst->prev->str, 0, 0);
			free(historyFirst->prev);
			historyFirst->prev=0;
		} else {
			free(historyFirst);
			historyFirst=0;
			historyLast=0;
		}
}

struct utilStrList *consoleGetHistory() {
	return historyFirst;
}



// -- command line and key events handling --

char *consoleInputLine = NULL;

static int cmdEnd=0;
static int cmdBegin=1;  // 0th is :
static int completionEnd=0;
struct utilStrList *completions=0;

static char cursor[]={
	consoleSpecialColorGray,
	'\x14',
	consoleSpecialColorNormal,
	consoleSpecialBack,
	'\0'};
static char cursorLen=sizeof(cursor)-1;

static void cmdLineRealloc(int newChars) {
	utilStrRealloc(&consoleInputLine, 0, (cmdEnd>completionEnd?cmdEnd:completionEnd)+newChars+1);
}

static void setCursorPos(int pos) {
	if (cursorPos) {
		utilStrRmChars(consoleInputLine+cursorPos, cursorLen);
		if (cursorPos<cmdEnd)
			cmdEnd-=cursorLen;
		if (completionEnd) {
			completionEnd-=cursorLen;
			consoleInputLine[cmdEnd]=consoleSpecialColorGray;
		}
	} else if(pos) {
		cmdLineRealloc(cursorLen);
	}
	cursorPos=pos;
	if (cursorPos) {
		utilStrInsertChars(consoleInputLine+cursorPos, cursor);
		if (cursorPos<=cmdEnd)
			cmdEnd+=cursorLen;
		else
			consoleInputLine[cursorPos+2]=consoleSpecialColorGray;
		if (completionEnd) {
			completionEnd+=cursorLen;
			if (cursorPos>cmdEnd)
				consoleInputLine[cmdEnd]=consoleSpecialColorBlue;
		}
	}
}
void showActiveCompletion() {
	consoleInputLine[cmdEnd]='\0';
	completionEnd=0;
	if (completions) {
		completionEnd=strlen(completions->str);
		cmdLineRealloc(completionEnd+2);
		consoleInputLine[cmdEnd]=consoleSpecialColorGray;
		strcpy(consoleInputLine+cmdEnd+1, completions->str);
		completionEnd+=1+cmdEnd;
	}
}

void updateCompletions() {
	consoleInputLine[cmdEnd]='\0';
	while (completions)
		utilStrListRm(&completions);
	if (cmdBegin<cmdEnd) {
		completions=cpComplete(consoleInputLine+cmdBegin);
	} else {
		completions=NULL;
	}
	showActiveCompletion();
}

void applyCompletion() {
	if (cursorPos>cmdEnd) {
		int pos=cursorPos;
		setCursorPos(0);
		consoleInputLine[pos]='\0';
		utilStrRmChars(consoleInputLine+cmdEnd, 1);
		cmdEnd=pos-1;
		setCursorPos(cmdEnd);
	} else {
		consoleInputLine[cmdEnd]='\0';
	}
	completionEnd=0;
}

static void openConsole(char *str) {
	historyActive=0;
	cursorPos=0;
	cmdLineRealloc(strlen(str));
	strcpy(consoleInputLine, str);
	cmdEnd=strlen(consoleInputLine);
	updateCompletions();
	setCursorPos(cmdEnd);
}

bool consoleIsOpen() {
	return cursorPos;
}


void consoleKeyPress(char c) {
	if (!cursorPos)
		openConsole("");
	applyCompletion();

	int pos=cursorPos;
	setCursorPos(0);
	cmdLineRealloc(1);
	utilStrInsertChar(consoleInputLine+pos, c);
	cmdEnd++;

	updateCompletions();

	setCursorPos(pos+1);
	//drawerInvokeRedisplay();
}

void consoleEsc() {
	historyActive=0;
	cursorPos=0;
}

void consoleBackspace() {
	if (cursorPos>cmdBegin) {
		if (cursorPos<=cmdEnd) {
			int pos=cursorPos;
			setCursorPos(0);
			utilStrRmChars(consoleInputLine + --pos, 1);
			cmdEnd--;
			updateCompletions();
			setCursorPos(pos);
		} else {
			setCursorPos(cmdEnd);
		}
	} else if (cursorPos+cursorLen==cmdEnd) {
		historyActive=0;
		cursorPos=0;
	}
	//drawerInvokeRedisplay();
}

void consoleDelete() {
	if ((cursorPos+cursorLen<cmdEnd) || (cursorPos+cursorLen<completionEnd)) {
		consoleRight();
		consoleBackspace();
	}
}

void consoleEnter() {
	applyCompletion();
	consoleInputLine[cmdEnd]='\0';
	setCursorPos(0);

	if (historyMaxCount) {
		if (historyCount<historyMaxCount)
			historyCount++;
		else
			utilStrListRm(&historyFirst);
		utilStrListAddAfter(&historyLast);
		if (!historyFirst)
			historyFirst=historyLast;
		utilStrRealloc(&historyLast->str, 0, cmdEnd+1);
		strncpy(historyLast->str, consoleInputLine, cmdEnd);
		historyLast->str[cmdEnd]='\0';
	}
	char *cmd=consoleInputLine;
	consoleInputLine=0;
	historyActive=0;
	cursorPos=0;
	if (!cpExecute(cmd+cmdBegin)) {; // skip :
		msgSend_printErr("Wrong command");
	}
	utilStrRealloc(&cmd, 0, 0);
	//drawerInvokeRedisplay();
}

void consoleUp() {
	if (!historyActive)
		historyActive=historyLast;
	else if (historyActive->prev)
		historyActive=historyActive->prev;
	if (historyActive) {
		struct utilStrList *active=historyActive;
		openConsole(active->str);
		historyActive=active;
		//drawerInvokeRedisplay();
	}
}

void consoleDown() {
	if (historyActive) {
		historyActive=historyActive->next;
		if (historyActive) {
			struct utilStrList *active=historyActive;
			openConsole(historyActive->str);
			historyActive=active;
		} else {
			openConsole(":");
		}
		//drawerInvokeRedisplay();
	}
}

void consoleLeft() {
	if (cursorPos>cmdBegin) {
		if (cursorPos==cmdEnd+1)
			setCursorPos(cursorPos-2);
		else
			setCursorPos(cursorPos-1);
	}
	//drawerInvokeRedisplay();
}

void consoleRight() {
	if ((cursorPos+cursorLen<cmdEnd) || (cursorPos+cursorLen<completionEnd)) {
		if (cursorPos+cursorLen==cmdEnd)
			setCursorPos(cursorPos+2);
		else
			setCursorPos(cursorPos+1);
	}
	//drawerInvokeRedisplay();
}

void consoleHome() {
	setCursorPos(cmdBegin);
	//drawerInvokeRedisplay();
}

void consoleEnd() {
	setCursorPos(0);
	if (completionEnd>cmdEnd)
		setCursorPos(completionEnd);
	else
		setCursorPos(cmdEnd);
	//drawerInvokeRedisplay();
}

void consoleTab() {
	if (completions) {
		int pos=cursorPos;
		setCursorPos(0);
		if (pos==completionEnd) {
			if (completions->next)
				completions=completions->next;
			else while (completions->prev)
				completions=completions->prev;
			showActiveCompletion();
		}
		if (*completions->str=='\0')
			setCursorPos(cmdEnd);
		else
			setCursorPos(completionEnd);
		//drawerInvokeRedisplay();
	}
}
