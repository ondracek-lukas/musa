// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <GL/freeglut.h>
#include <GL/gl.h>

#include "player.h"
#include "drawerBuffer.h"
#include "drawerMusicVisualiser.h"
#include "drawerScale.h"
#include "drawer.h"
#include "consoleIn.h"
#include "consoleOut.h"
#include "consoleStatus.h"

#include "util.h"
#include "taskManager.h"
#include "messages.h"

struct taskInfo drawerMainTask    = TM_TASK_INITIALIZER(true, true);
struct taskInfo drawerConsoleTask = TM_TASK_INITIALIZER(true, true);


static size_t width, height;
static size_t rowBegin=0;
static size_t newColumns=0;
static size_t inputColumnLen=1;
static size_t inputColumnLenToDraw=1;

static GLfloat stringColor[]      = {1.0f,  1.0f, 1.0f};
static GLfloat stringColorRed[]   = {1.0f,  0.3f, 0.3f};
static GLfloat stringColorGreen[] = {0.05f, 0.9f, 0.05f};
static GLfloat stringColorBlue[]  = {0.5f,  0.5f, 1.0f};
static GLfloat stringColorGray[]  = {0.5f,  0.5f, 0.5f};

float keyboardMinFreq;
unsigned keyboardMinTone;
double centeredRatio;

#define SCALE_LINES_ALPHA_MAIN 0.5
#define SCALE_LINES_ALPHA_OTHER 0.3
#define MIN_SCALE_LABELS_DIST 50 // px
#define SCALE_LABELS_FONT_HEIGHT 15 // px
#define SCALE_LABELS_FONT_HEIGHT_UP 12 // px
#define SCALE_LABELS_FONT GLUT_BITMAP_HELVETICA_12
#define KEYBOARD_ALPHA 0.5

bool repaintInvoked = false;
void drawerInvokeRepaint() {
	repaintInvoked = true;
}

static void onTimer();
static void onReshape(int w, int h);
static void onDisplay();

void drawerInit() {

	dbufferInit();
	dbufferMove(DRAWER_BUFFER_SIZE/2);

	dmvInit();

	glutReshapeFunc(onReshape);
	glutDisplayFunc(onDisplay);
	glutTimerFunc(DRAWER_FRAME_DELAY, onTimer, 0);
}


// --- DRAWING ---

int drawerBufferPos=0;
int screenCenterColumn=0;
bool drawOnlyControls = false;
bool forceMain = false;
bool forceNext = false;
bool afterReset = false;

void drawerReset() {
	screenCenterColumn=width*msgOption_cursorPos/100;
	msgSet_visibleBefore(screenCenterColumn);
	msgSet_visibleAfter(width-screenCenterColumn);
	forceNext = true;
	afterReset = true;
}

// -- controls --

static void drawRect(int x1, int y1, int x2, int y2);
static void drawString(char *string, int x, int y);
static struct utilStrList *drawStringMultiline(struct utilStrList *lines, int count, int x, int y);
//static void drawBlock();
static void drawStatusLine();

static void drawRect(int x1, int y1, int x2, int y2) {
	glBegin(GL_QUADS);
	glVertex2i(x1, y1);
	glVertex2i(x1, y2);
	glVertex2i(x2, y2);
	glVertex2i(x2, y1);
	glEnd();
}

static void drawString(char *str, int x, int y) {
	//GLfloat color[4];
	//glGetFloatv(GL_CURRENT_COLOR, color);

	//glColor4f(0, 0, 0, 0.8);
	//drawRect(x-1, y-5, x+consoleStrWidth(str)*9+1, y+14);
	//glColor3fv(color);
	glRasterPos2i(x, y);


	while (*str) {
		switch (*str) {
			case consoleSpecialBack:
				x-=9;
				glRasterPos2i(x, y);
				break;
			case consoleSpecialColorNormal:
				glColor3fv(stringColor);
				glRasterPos2i(x, y);
				break;
			case consoleSpecialColorRed:
				glColor3fv(stringColorRed);
				glRasterPos2i(x, y);
				break;
			case consoleSpecialColorGreen:
				glColor3fv(stringColorGreen);
				glRasterPos2i(x, y);
				break;
			case consoleSpecialColorBlue:
				glColor3fv(stringColorBlue);
				glRasterPos2i(x, y);
				break;
			case consoleSpecialColorGray:
				glColor3fv(stringColorGray);
				glRasterPos2i(x, y);
				break;
				break;
			default:
				x+=9;
				glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *str);
				break;
		}
		str++;
	}
}

static struct utilStrList *drawStringMultiline(struct utilStrList *lines, int count, int x, int y) {
	int i;
	for (i=0; lines && (!count || i<count); i++, lines=lines->next) {
		drawString(lines->str, x, y);
		y-=19;
	}
	return lines;
}
/*
static void drawBlock() {
	int x, y;
	struct utilStrList *list=0;
	if (!consoleBlock)
		return;
	x=(width-consoleBlockWidth*9)/2;
	y=(height+consoleBlockHeight*19)/2-10;

	if ((x<0) || (y>height-10-19)) {
		glColor3fv(stringColorRed);
		utilStrListAddAfter(&list);
		utilStrRealloc(&list->str, 0, 20);
		strcpy(list->str, "Too small window");
		utilStrListAddAfter(&list);
		utilStrRealloc(&list->str, 0, 20);
		sprintf(list->str, "%dx%d needed", consoleBlockWidth*9, consoleBlockHeight*19+2*19);
		utilStrListAddAfter(&list);
		utilStrRealloc(&list->str, 0, 20);
		sprintf(list->str, "current: %dx%d", width, height);
		list=list->prev;
		list=list->prev;
		x=(width>17*9?(width-17*9)/2:0);
		y=(height>3*19?(height+3*19)/2:height)-12;
		drawStringMultiline(list, 0, x, y);
		while (list)
			utilStrListRm(&list);
	} else {
		glColor3fv(stringColor);
		drawStringMultiline(consoleBlock, 0, x, y);
	}
}
*/

static void drawLogo() { // TODO create logo
	glColor4f(0, 0, 0, 1);
	drawRect(0, 0, width, height);
}

static void drawStatusLine() {
	int consoleSize=0, count=0;
	struct utilStrList *lines=NULL;

	if ((lines=consoleLines)) {
		consoleSize=consoleStrWidth(lines->str);
		count=1;
		while (lines->prev) {
			count++;
			lines=lines->prev;
		}
	}
	if (consoleIsOpen()) count++;

	glColor4f(0, 0, 0, 1);
	if (count>1) {
		drawRect(0, 0, width, 20*count);
		drawOnlyControls = true;
	} else {
		drawRect(0, 0, width, 20);
		forceMain |= drawOnlyControls;
		drawOnlyControls = false;
	}

	glColor3fv(stringColor);
	if (lines) {
		drawStringMultiline(lines, 0, 1, 19*count-14);
	}
	if (consoleIsOpen()) {
		int lineSize = consoleStrWidth(consoleInputLine);
		if (lineSize>consoleSize) consoleSize = lineSize;
		drawString(consoleInputLine, 1, 5);
	}

	char *status = consoleStatusGet();
	glColor3fv(stringColorGreen);
	if (9*(strlen(status)+consoleSize+3)<=width)
		drawString(status, width-9*strlen(status)-10, 5);
	else if (!consoleLines)
		drawString(status, 10, 5);

}

// -- rest --


static inline void drawColumnColors(int screenColumn, unsigned char *colors) {
	glRasterPos2i(screenColumn,20);
	glDrawPixels(1, height-20, GL_RGB, GL_UNSIGNED_BYTE, colors);
}
static inline void drawColumnColor(int screenColumn, GLbyte *color) {
	glColor3ubv(color);
	glLineWidth(1);
	glBegin(GL_LINES);
	glVertex2i(screenColumn, 20);
	glVertex2i(screenColumn, height);
	glEnd();
}
static inline void drawColumnOverlay(int screenColumn, GLbyte *colorsA) {
	glRasterPos2i(screenColumn,20);
	glDrawPixels(1, height-20, GL_RGBA, GL_UNSIGNED_BYTE, colorsA);
}
static inline void drawColumn(int screenColumn, bool scaleOnly) {
	int column = screenColumn - screenCenterColumn + drawerBufferPos;
	char *colors=NULL;
	if (dbufferExists(column)) {
		dbufferDrawn(column) = true;
		__sync_synchronize();
		if (!scaleOnly && (dbufferPrecision(column) > 0)) {
			colors=&dbuffer(column, 0, 0);
			drawColumnColors(screenColumn, colors);
		} else {
			drawColumnColor(screenColumn, (unsigned char []){0, 0, 0});
			if (scaleOnly && (dbufferPrecision(column) > 0)) {
				dbufferDrawn(column) = false;
			}
		}
	} else {
		drawColumnColor(screenColumn, (unsigned char []){0, 255, 255});
	}
	if (!dbuffer.dataInvalid) {
		drawColumnOverlay(screenColumn, dbuffer.columnOverlay);
	}
}
static inline void moveColumns(int offset) {
	int cnt, from, to;
	if (offset > 0) {
		cnt  = width - offset; from = offset; to = 0;
	} else {
		cnt  = width + offset; from = 0; to = -offset;
	}
	glRasterPos2i(to, 20);
	glCopyPixels(from, 20, cnt, height-20, GL_COLOR);
}

// Draws the view, possibly moving some area (if not forced)
static bool repaint(bool force) {
	if (afterReset) {
		afterReset = false;
		drawLogo();
		glFlush();
		glutSwapBuffers();
		drawLogo();
		return false;
	}

	int time = glutGet(GLUT_ELAPSED_TIME);

	{ bool tmp;
		tmp = forceNext;
		forceNext = force;
		force |= tmp;
	}

	if (force && drawOnlyControls) {
		drawLogo();
	}

	if (tmTaskEnter(&drawerConsoleTask)) {
		drawStatusLine();
		tmTaskLeave(&drawerConsoleTask);
	}

	force |= forceMain;
	forceMain = false;

	bool interrupted = false;
	if (!drawOnlyControls && tmTaskEnter(&drawerMainTask)) {
		static int oldBufferPos=0;
		int offset=drawerBufferPos-oldBufferPos;
		if (offset > 0) {
			for (int i = oldBufferPos-screenCenterColumn; i<drawerBufferPos-screenCenterColumn; i++) {
				dbufferDrawn(i) = false;
			}
		} else {
			for (int i = drawerBufferPos+width-screenCenterColumn+1; i<=oldBufferPos+width-screenCenterColumn; i++) {
				dbufferDrawn(i) = false;
			}
		}
		if (!force && (abs(offset) < width)) {
			moveColumns(offset);
		}

		int cnt=0;
		for (int i=0; i<width; i++) {

			int j = (i < width - screenCenterColumn ? i + screenCenterColumn : width - i - 1);

			if (force || !dbufferDrawn(j+drawerBufferPos-screenCenterColumn)) {
				if (!interrupted && (++cnt % 16 == 0) && (glutGet(GLUT_ELAPSED_TIME)-time > DRAWER_MAX_PAINT_TIME)) {
					interrupted = true;
				}
				if (interrupted) {
					dbufferDrawn(j+drawerBufferPos-screenCenterColumn) = false;
				} else {
					drawColumn(j, false);
				}
			}
		}

		oldBufferPos = drawerBufferPos;
		tmTaskLeave(&drawerMainTask);
	} else {
		forceMain |= force;
	}

	glFlush();
	glutSwapBuffers();
	return !interrupted;
}

// --- EVENTS ---

static void onTimer() {
	int newPos=dsPlayerPosToColumn(playerPos);

	dbufferMove(newPos-drawerBufferPos);
	drawerBufferPos = newPos;
	dmvRefresh();
	bool done=repaint(false);

	if (done) {
		glutTimerFunc(DRAWER_FRAME_DELAY, onTimer, 0);
	} else {
		glutTimerFunc(0, onTimer, 0);
	}
}


static void onReshape(int w, int h) {
	width=w;
	height=h;

	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, 0, h, -1, 1);
	glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glTranslatef(0.375, 0.375, 0.);

	msgSet_columnLen(h-20);

	glReadBuffer(GL_FRONT);
	glDrawBuffer(GL_BACK);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	drawerReset();
}

static void onDisplay() {
	repaint(true);
}
