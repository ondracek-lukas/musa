// MusA               Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file
// Geometric Figures  Copyright (C) 2015--2016  Lukáš Ondráček <ondracek.lukas@gmail.com>

// consoleOut takes care of printing text to user.

// To be used only from one thread at a time:
// direct access is allowed only from event-dispatcher thread under drawerConsoleTask,
// use messages module to call functions with paused task.

#ifndef CONSOLE_OUT_H
#define CONSOLE_OUT_H

#include <GL/freeglut.h>
#include <stdbool.h>
#include "consoleShared.h"


struct utilStrList;


/*
// Prints block from stringsData, returns false if doesn't exist
extern bool consolePrintNamedBlock(char *section, char *name);

// Prints block from string
extern void consolePrintBlock(char *str);

// Clears printed block
extern void consoleClearBlock();
*/

// Prints list of lines.
extern void consolePrintLinesList(struct utilStrList *lines);

// Prints line instead (or below in multiline mode) the previous one.
extern void consolePrint(char *string);

// Prints one line with error message.
extern void consolePrintErr(char *string);

// Clears printed line(s).
extern void consoleClear();

// Printing will clear previously printed lines.
extern void consoleClearBeforePrinting();

// Appends msg line and clears only it while typing command.
extern void consoleClearAfterCmd(char *msg);
extern void consoleClearAfterCmdDefaultMsg();



// drawer interface, read-only, use under drawerConsoleTask.
extern struct utilStrList *consoleLines;
extern struct utilStrList *consoleBlock;
extern int consoleBlockWidth;
extern int consoleBlockHeight;


#endif
