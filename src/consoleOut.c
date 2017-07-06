// MusA               Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file
// Geometric Figures  Copyright (C) 2015--2016  Lukáš Ondráček <ondracek.lukas@gmail.com>

#include "consoleOut.h"

#include <GL/freeglut.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>

#include "util.h"

static __attribute__((constructor)) void init() {
	utilStrRealloc(&consoleStatus, 0, 32);
	*consoleStatus='\0';
}

int consoleStrWidth(char *str) {
	int width=0;
	int lineWidth=0;
	for (; *str; str++) {
		switch (*str) {
			case '\n':
				if (width<lineWidth)
					width=lineWidth;
				lineWidth=0;
				break;
			case consoleSpecialBack:
				lineWidth--;
				break;
			case consoleSpecialColorNormal:
			case consoleSpecialColorRed:
			case consoleSpecialColorGreen:
			case consoleSpecialColorGray:
				break;
			default:
				lineWidth++;
		}
	}
	if (width<lineWidth)
		width=lineWidth;
	return width;
}
int consoleStrHeight(char *str) {
	int height=1;
	for (; *str; str++)
		if (*str=='\n')
			height++;
	return height;
}



/*
// -- block printing --

struct utilStrList *consoleBlock=0;
int consoleBlockWidth;
int consoleBlockHeight;
bool consolePrintNamedBlock(char *section, char *name) {
	consoleClearBlock();
	consoleBlock=stringsGet(section, name, &consoleBlockWidth, &consoleBlockHeight);
	if (consoleBlock)
		drawerInvokeRedisplay();
	return consoleBlock;
}
void consolePrintBlock(char *str) {
	consoleClearBlock();
	consoleBlockWidth=consoleStrWidth(str);
	consoleBlockHeight=consoleStrHeight(str);
	struct utilStrList *lines=utilStrListOfLines(str);
	for (struct utilStrList *line=lines; line; line=line->next) {
		int i=strlen(line->str);
		int newLen=consoleBlockWidth-consoleStrWidth(line->str)+i;
		utilStrRealloc(&line->str, 0, newLen+1);
		for (; i<newLen; i++)
			line->str[i]=' ';
		line->str[i]='\0';
	}
	consoleBlock=lines;
	//drawerInvokeRedisplay();
}

void consoleClearBlock() {
	//if (consoleBlock)
	//	drawerInvokeRedisplay();
	while (consoleBlock)
		utilStrListRm(&consoleBlock);
}
*/

// -- status line --

char *consoleStatus=0;

void consolePrintStatus(char *string) {
	if (strcmp(consoleStatus, string)!=0) { // redisplay only if needed
		utilStrRealloc(&consoleStatus, 0, strlen(string)+1);
		strcpy(consoleStatus, string);
		//drawerInvokeRedisplay();
	}
}


// -- command line printing --

struct utilStrList *consoleLines=0;
static enum mode {
	modeNormal,
	modeRewriteLast,
	modeAppend
} mode = modeNormal;

void consolePrint(char *str) {
	consolePrintLinesList(utilStrListOfLines(str));
}

void consolePrintLinesList(struct utilStrList *lines) {
	if (mode!=modeAppend) {
		consoleClear();
		mode=modeAppend;
	}
	if (lines) {
		while (lines->prev)
			lines=lines->prev;
		utilStrListCopyAfter(&consoleLines, lines);

		struct utilStrList *lines2=consoleLines;
		while ((lines=lines->next)) {
			lines2=lines2->prev;
		}

		utilStrRealloc(&lines2->str, 0, strlen(lines2->str)+2);
		utilStrInsertChar(lines2->str, consoleSpecialColorNormal);
		//drawerInvokeRedisplay();
	}
}

void consolePrintErr(char *str) {
	static char *str2=0;
	utilStrRealloc(&str2, 0, strlen(str)+2);
	*str2=consoleSpecialColorRed;
	strcpy(str2+1, str);
	consolePrint(str2);
}

void consoleClear() {
	mode=modeNormal;
	while (consoleLines)
		utilStrListRm(&consoleLines);
	//consoleClearBlock();
}

void consoleClearBeforePrinting() {
	if (mode == modeAppend)
		mode=modeNormal;
}
void consoleClearAfterCmdDefaultMsg() {
	consoleClearAfterCmd("Press any key or type command to continue");
}
void consoleClearAfterCmd(char *msg) {
	mode=modeAppend;
	consolePrint(msg);
	mode=modeRewriteLast;
}
