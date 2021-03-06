// MusA               Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file
// Geometric Figures  Copyright (C) 2015--2016  Lukáš Ondráček <ondracek.lukas@gmail.com>

#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>

#define utilExitErr(format,...) { \
	fprintf(stderr, "Error occurred: " format "\n", ##__VA_ARGS__); \
	abort(); \
}



// Expands path starting with ~/ (home directory) or %/ (app location)
// Returned path is valid till next call (if expanded)
extern char *utilExpandPath(char *ipath);

// Returns path of the application
extern char *utilExecutablePath();

// Returns pointer to a filename in the given path
char *utilFileNameFromPath(char *path);


// Inserts given character before given null terminated string
extern void utilStrInsertChar(char *str, char c);

// Inserts given chars string before given null terminated str string
extern void utilStrInsertChars(char *str, char *chars);

// Removes given number of characters from the beginning of the string
extern void utilStrRmChars(char *str, int cnt);


// List of strings:
	struct utilStrList {
		char *str;
		struct utilStrList *prev;
		struct utilStrList *next;
	};
	// Adds new node after given list, use utilStrRealloc(node->str, ...) before using
	// list will point to the new node
	extern void utilStrListAddAfter(struct utilStrList **list);

	// Adds copy of the given list after given node
	// after will point to the last element of list
	extern void utilStrListCopyAfter(struct utilStrList **after, struct utilStrList *list);

	// Adds the given list after given node
	// after will point to the last element of list
	// list will be still valid pointer
	extern void utilStrListMoveAfter(struct utilStrList **after, struct utilStrList *list);

	// Creates list of lines of given string
	extern struct utilStrList *utilStrListOfLines(char *str);

	// Removes given node from its list, frees node->str
	// list will point to the next element if exists, otherwise to the previous one
	extern void utilStrListRm(struct utilStrList **list);


// POSIX stpcpy reimplemented for Windows
extern char *utilStpcpy(char *dest, const char *src);
#ifdef WIN32
#define stpcpy utilStpcpy
#endif

// Portable function to suspend thread
extern void utilSleep(int ms);

#endif
