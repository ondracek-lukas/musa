// MusA               Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file
// Geometric Figures  Copyright (C) 2015--2016  Lukáš Ondráček <ondracek.lukas@gmail.com>

// consoleIn manages text input from user.

// To be used only from event-dispatcher thread.


#ifndef CONSOLE_IN_H
#define CONSOLE_IN_H

#include <stdbool.h>
#include "consoleShared.h"

// Returns whether input console line is open (: was pressed)
extern bool consoleIsOpen();

// Key press events handling on console
extern void consoleKeyPress(char c);
extern void consoleEsc();
extern void consoleBackspace();
extern void consoleDelete();
extern void consoleEnter();
extern void consoleUp();
extern void consoleDown();
extern void consoleLeft();
extern void consoleRight();
extern void consoleHome();
extern void consoleEnd();
extern void consoleTab();

// Gets/sets the length of the history
extern int consoleGetHistoryMaxCount();
extern void consoleSetHistoryMaxCount(int maxCount);

// Gets the history
extern struct utilStrList *consoleGetHistory();


// drawer interface, read-only
extern int consoleInLength;
extern char *consoleInputLine;

#endif
