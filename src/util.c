// MusA               Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file
// Geometric Figures  Copyright (C) 2015--2016  Lukáš Ondráček <ondracek.lukas@gmail.com>


#include "util.h"
#include "mem.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef WIN32
#include <time.h>
#endif
#include <GL/freeglut.h>
#include <math.h>
#include <stdbool.h>




// -- paths --

char *utilExpandPath(char *ipath) { // returned path is valid till next call (if expanded)
	static char *opath=0;
	static int opathSize=0;
	char *prefix=0;
	int prefixLength=0;
	int length;

#ifdef WIN32
	if ((strncmp(ipath, "~\\",2)==0)||(strncmp(ipath,"~/",2)==0)){
		prefix=getenv("userprofile");
		if (prefix)
			prefixLength=strlen(prefix);
	} else if ((strncmp(ipath, "%\\",2)==0)||(strncmp(ipath,"%/",2)==0)){
		prefix=utilExecutablePath();
		if (prefix)
			prefixLength=strrchr(prefix, '\\')-prefix;
	}
#else
	if (strncmp(ipath, "~/",2)==0) {
		prefix=getenv("HOME");
		if (prefix)
			prefixLength=strlen(prefix);
	} else if (strncmp(ipath, "%/",2)==0) {
		prefix=utilExecutablePath();
		if (prefix)
			prefixLength=strrchr(prefix, '/')-prefix;
	}
#endif

	if (!prefix)
		return ipath;

	length=prefixLength+strlen(ipath);
	if (opathSize<length) {
		opathSize=length;
		opath=memRealloc(opath, opathSize*sizeof(char));
	}
	strncpy(opath, prefix, prefixLength);
#ifdef WIN32
	opath[prefixLength]='\\';
#else
	opath[prefixLength]='/';
#endif
	strcpy(opath+prefixLength+1, ipath+2);
	return opath;
}

char *utilExecutablePath() {
	static char *path=0;
	int size=16;
	int length=0;

	if (!path) {
		do {
			path=memRealloc(path, sizeof(char)*(size*=2));
#ifdef WIN32
			length=GetModuleFileName(0, path, size);
#else
			length=readlink("/proc/self/exe", path, size);
			if (length<0)
				length=readlink("/proc/curproc/file", path, size);
#endif
		} while (length>=size);

		if (length>0) {
			path[length]='\0';
		} else {
			free(path);
			path=0;
		}
	}
	return path;
}

char *utilFileNameFromPath(char *path) {
	char *name=strrchr(path, '/');
#ifdef WIN32
	char *backslash=strrchr(path, '\\');
	if (backslash>name)
		name=backslash;
#endif
	if (name) {
		return name+1;
	} else {
		return path;
	}
}



// -- string utils --

void utilStrInsertChar(char *str, char c) {
	char chars[2];
	chars[0]=c;
	chars[1]='\0';
	utilStrInsertChars(str, chars);
}

void utilStrInsertChars(char *str, char *chars) {
	int cnt=strlen(chars);
	char *str2=str;
	while (*str2++);
	while (str2--!=str)
		*(str2+cnt)=*str2;
	while (*chars)
		*str++ = *chars++;
}

void utilStrRmChars(char *str, int cnt) {
	do {
		*str=str[cnt];
	} while (*str++);
}


// -- string list --

void utilStrListAddAfter(struct utilStrList **pAfter) {
	struct utilStrList *new=memMalloc(sizeof(struct utilStrList));
	new->str=0;
	new->prev=*pAfter;
	if (*pAfter)
		new->next=(*pAfter)->next;
	else
		new->next=0;
	if (*pAfter && (*pAfter)->next)
		(*pAfter)->next->prev=new;
	if (*pAfter)
		(*pAfter)->next=new;
	*pAfter=new;
}

void utilStrListCopyAfter(struct utilStrList **pAfter, struct utilStrList *list) {
	while (list) {
		utilStrListAddAfter(pAfter);
		memStrRealloc(&(*pAfter)->str, 0, strlen(list->str)+1);
		strcpy((*pAfter)->str, list->str);
		list=list->next;
	}
}

void utilStrListMoveAfter(struct utilStrList **pAfter, struct utilStrList *list) {
	if (!list)
		return;
	struct utilStrList *before=NULL;
	if (*pAfter)
		before=(*pAfter)->next;
	if (list->prev)
		list->prev->next=NULL;
	list->prev=*pAfter;
	if (*pAfter)
		(*pAfter)->next=list;
	else
		*pAfter=list;
	while ((*pAfter)->next)
		*pAfter=(*pAfter)->next;
	(*pAfter)->next=before;
}

struct utilStrList *utilStrListOfLines(char *str) {
	struct utilStrList *lines=0;
	char *lineEnd;
	char *str2;
	do {
		lineEnd=str;
		utilStrListAddAfter(&lines);
		while (*lineEnd && (*lineEnd!='\n'))
			lineEnd++;
		memStrRealloc(&lines->str, 0, lineEnd-str+1);
		str2=lines->str;
		while (str!=lineEnd)
			*str2++ = *str++;
		*str2++='\0';
	} while (*str++);
	if (lines)
		while (lines->prev)
			lines=lines->prev;
	return lines;
}

void utilStrListRm(struct utilStrList **pList) {
	struct utilStrList *node=*pList;
	if (node->next)
		node->next->prev=node->prev;
	if (node->prev)
		node->prev->next=node->next;
	if (node->next)
		*pList=node->next;
	else
		*pList=node->prev;
	memStrRealloc(&node->str, 0, 0);
	free(node);
}

// -- other portable functions --

char *utilStpcpy(char *dest, const char *src) {
	do {
		*dest=*src;
	} while (dest++, *src++);
	return dest-1;
}

void utilSleep(int ms) {
#ifdef WIN32
	Sleep(ms);
#else
	struct timespec sleepTime;
	sleepTime.tv_sec=0;
	sleepTime.tv_nsec=ms*1000000;
	nanosleep(&sleepTime, 0);
#endif
}


