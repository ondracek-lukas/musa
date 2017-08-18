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


#define FRAME_DELAY 10   // ms
#define MAX_PAINT_TIME 50   // ms

static GLfloat cursorColor[]      = {0.05f, 0.9f, 0.05f, 0.7f};
static GLfloat stringColor[]      = {1.0f,  1.0f, 1.0f};
static GLfloat stringColorRed[]   = {1.0f,  0.3f, 0.3f};
static GLfloat stringColorGreen[] = {0.05f, 0.9f, 0.05f};
static GLfloat stringColorBlue[]  = {0.5f,  0.5f, 1.0f};
static GLfloat stringColorGray[]  = {0.5f,  0.5f, 0.5f};

#define STATUS_LINE_HEIGHT 20

struct taskInfo drawerMainTask    = TM_TASK_INITIALIZER(true, true);
struct taskInfo drawerConsoleTask = TM_TASK_INITIALIZER(true, true);

double drawerVisibleBegin=0, drawerVisibleEnd=0; // sec

static size_t width=0, height=0, soundHeight=0, consoleHeight=STATUS_LINE_HEIGHT;
static int columnsCnt = 0, firstColumn = 0, visibleColumnsCnt = 0, cursorPos = 0;
#define screenColumn(column) ((msgOption_reverseDirection ? visibleColumnsCnt - ((column)-firstColumn) - 1 : (column)-firstColumn) + (msgOption_swapAxes * STATUS_LINE_HEIGHT))
static bool *drawnColumns = NULL;
#define drawnColumns(i) drawnColumns[((i) + columnsCnt) % columnsCnt]
static bool cursorDrawn = false;

static bool repaintInterrupted = false;
static bool repaintAllColumns = false;


static void onTimer();
static void onReshape(int w, int h);
static void onDisplay();
static void repaint(bool force);
static void drawConsole(bool force);

void drawerInit() {
	glutReshapeFunc(onReshape);
	glutDisplayFunc(onDisplay);
	glutTimerFunc(FRAME_DELAY, onTimer, 0);
}

static void onReshape(int w, int h) {
	width=w;
	height=h;

	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, 0, h, -1, 1);
	glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glTranslatef(0.375, 0.375, 0.);

	glReadBuffer(GL_FRONT);
	glDrawBuffer(GL_BACK);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	drawerReset();
}

void drawerReset() {
	int screenCenterColumn=width*msgOption_cursorPos/100;

	soundHeight = height - STATUS_LINE_HEIGHT;

	int newColumnsCnt;
	if (msgOption_swapAxes) {
		newColumnsCnt = soundHeight;
		visibleColumnsCnt = 0;
		msgSet_columnLen(width);
	} else {
		newColumnsCnt = width;
		visibleColumnsCnt = width;
		msgSet_columnLen(height-STATUS_LINE_HEIGHT);
	}
	if (newColumnsCnt != columnsCnt) {
		columnsCnt = newColumnsCnt;
		free(drawnColumns);
		drawnColumns = utilMalloc(sizeof(bool) * columnsCnt);
	}
	cursorDrawn = false;
	repaintInterrupted = false;
	cursorPos = 0;
	drawerVisibleBegin = 0;
	drawerVisibleEnd = 0;


	for (int i = firstColumn; i < firstColumn + columnsCnt; i++) {
		drawnColumns(cursorPos) = false;
	}
}

static void onTimer() {
	repaint(false);

	if (repaintInterrupted) {
		glutTimerFunc(0, onTimer, 0);
	} else {
		glutTimerFunc(FRAME_DELAY, onTimer, 0);
	}
}


static void onDisplay() {
	repaint(true);
}



static void drawRect(int x1, int y1, int x2, int y2) { // right and top boudary exclusive
	glBegin(GL_QUADS);
	glVertex2i(x1,   y1);
	glVertex2i(x1,   y2);
	glVertex2i(x2  , y2);
	glVertex2i(x2,   y1);
	glEnd();
}
static void copyRect(int x1, int y1, int x2, int y2) { // right and top boudary exclusive
	glRasterPos2i(x1, y1);
	glCopyPixels(x1, y1, x2-x1, y2-y1, GL_COLOR);
}

static void drawString(char *str, int x, int y) {
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

static void drawConsole(bool force) {
	if (tmTaskEnter(&drawerConsoleTask)) {
		int consoleSize=0, count=0;
		struct utilStrList *lines=NULL;

		if ((lines=consoleLines)) {
			consoleSize=consoleStrWidth(lines->str); // characters on the last line
			count=1;
			while (lines->prev) {
				count++;
				lines=lines->prev;
			}
		}
		if (consoleIsOpen()) count++;

		if (count < 1) count = 1;
		glColor4f(0, 0, 0, 1);
		int newConsoleHeight = 19*count+1;
		if (newConsoleHeight < consoleHeight) {
			repaintAllColumns = true;
		}
		consoleHeight = newConsoleHeight;
		drawRect(0, 0, width, consoleHeight);

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
		int maxLen = width/9 - consoleSize - 4;
		int statusLen = strlen(status);
		if (statusLen <= maxLen) {
			drawString(status, width-9*statusLen-10, 5);
		} else if (maxLen-3 > 0) {
			drawString("...", width-9*maxLen-10, 5);
			drawString(status + statusLen - maxLen + 3, width-9*(maxLen-3)-10, 5);
		}
		tmTaskLeave(&drawerConsoleTask);
	} else if (!force) {
		copyRect(0, 0, width, consoleHeight);
	}
}


static inline void drawColumnColors(int screenColumn, GLbyte *colors) {
	if (msgOption_swapAxes) {
		glRasterPos2i(0, screenColumn);
		glDrawPixels(width, 1, GL_RGB, GL_UNSIGNED_BYTE, colors);
	} else {
		glRasterPos2i(screenColumn, consoleHeight);
		glDrawPixels(1, height - consoleHeight, GL_RGB, GL_UNSIGNED_BYTE, colors + 3*(consoleHeight-STATUS_LINE_HEIGHT));
	}
}
static inline void drawColumnOverlay(int screenColumn, GLbyte *colorsA) {
	if (msgOption_swapAxes) {
		glRasterPos2i(0, screenColumn);
		glDrawPixels(width, 1, GL_RGBA, GL_UNSIGNED_BYTE, colorsA);
	} else {
		glRasterPos2i(screenColumn, consoleHeight);
		glDrawPixels(1, height - consoleHeight, GL_RGBA, GL_UNSIGNED_BYTE, colorsA + 4*(consoleHeight-STATUS_LINE_HEIGHT));
	}
}
static inline void drawColumnColor(int screenColumn) {
	glLineWidth(1);
	glBegin(GL_LINES);
	if (msgOption_swapAxes) {
		glVertex2i(0, screenColumn);
		glVertex2i(width - 1, screenColumn);
	} else {
		glVertex2i(screenColumn, consoleHeight);
		glVertex2i(screenColumn, height - 1);
	}
	glEnd();
}
static inline void moveView(int offset, bool force) {
	if (!force) {
		int cnt, from, to;
		if (offset > 0) {
			cnt  = visibleColumnsCnt - offset; from = offset; to = 0;
		} else {
			cnt  = visibleColumnsCnt + offset; from = 0; to = -offset;
		}
		if (msgOption_reverseDirection) {
			int tmp = from; from = to; to = tmp;
		}
		if (msgOption_swapAxes) {
			glRasterPos2i(0, consoleHeight+to);
			glCopyPixels(0, consoleHeight+from, width, cnt, GL_COLOR);
		} else {
			glRasterPos2i(to, consoleHeight);
			glCopyPixels(from, consoleHeight, cnt, height-consoleHeight, GL_COLOR);
		}
	}
	firstColumn += offset;
	if (!force) {
		if (offset > 0) {
			for (int i = firstColumn + visibleColumnsCnt - offset; i < firstColumn + columnsCnt; i++) {
				drawnColumns(i) = false;
			}
		} else {
			for (int i = firstColumn; i < firstColumn-offset; i++) {
				drawnColumns(i) = false;
			}
		}
	}
}
static inline bool drawColumn(int column) {
	if (!drawnColumns(column)) {
		char *colors=NULL;
		if (dbufferExists(column)) {
			dbufferDrawn(column) = true;
			__sync_synchronize();
			if (dbufferPrecision(column) > 0) {
				drawColumnColors(screenColumn(column), &dbuffer(column, 0, 0));
			} else {
				glColor3f(0, 0, 0);
				drawColumnColor(screenColumn(column));
			}
			drawnColumns(column) = true;
		} else {
			glColor3f(0, 0, 0);
			drawColumnColor(screenColumn(column));
		}
		if (msgOption_showGrid) {
			drawColumnOverlay(screenColumn(column), dbuffer.columnOverlay);
		}
		return true;
	} else {
		return false;
	}
}
static void repaint(bool force) {

	int time = glutGet(GLUT_ELAPSED_TIME);
	glColor3f(0, 0, 0);
	drawRect(0, 0, width, height);


	if (playerSourceType) {
		if (tmTaskEnter(&drawerMainTask)) {
			if (force) {
				for (int i = firstColumn; i < firstColumn + columnsCnt; i++) {
					drawnColumns(i) = false;
				}
			} else {
				if (cursorDrawn) {
					drawnColumns(cursorPos) = false;
				}
			}
			int oldCursorPos = cursorPos;
			cursorPos = msgOption_outputRate * playerPosSec;
			int screenCursorPos = columnsCnt * msgOption_cursorPos/100;
			if (!msgOption_forceCursorPos) {
				int totalColumnsCnt = playerDuration * msgOption_outputRate;
				if (totalColumnsCnt < columnsCnt) {
					screenCursorPos = cursorPos + (columnsCnt - totalColumnsCnt)/2;
				} else if (screenCursorPos > cursorPos) {
					screenCursorPos = cursorPos;
				} else if (columnsCnt - screenCursorPos > totalColumnsCnt - cursorPos) {
					screenCursorPos = cursorPos + columnsCnt - totalColumnsCnt;
				}
			}
			int offset = cursorPos - screenCursorPos - firstColumn;

			moveView(offset, force);

			drawerVisibleBegin = firstColumn / msgOption_outputRate;
			drawerVisibleEnd = (firstColumn + columnsCnt) / msgOption_outputRate;

			drawConsole(force);

			if (repaintAllColumns) {
				repaintAllColumns = false;
				for (int i = firstColumn; i < firstColumn + columnsCnt; i++) {
					drawnColumns(i) = false;
				}
			}

			repaintInterrupted = false;
			int firstVisibleColumn = firstColumn;
			if (msgOption_swapAxes) {
				if (!msgOption_reverseDirection) {
					firstVisibleColumn += consoleHeight - STATUS_LINE_HEIGHT;
				}
				visibleColumnsCnt = columnsCnt + consoleHeight - STATUS_LINE_HEIGHT;
			}

			if (!repaintInterrupted) {
				for (int i = firstColumn; i < firstColumn + visibleColumnsCnt; i++) {
					if (!dbufferDrawn(i)) {
						drawnColumns(i) = false;
					}
				}
			}

			int cnt = 0;
			if (cursorDrawn && (oldCursorPos >= firstVisibleColumn) && (oldCursorPos < firstVisibleColumn + visibleColumnsCnt)) {
				if (!drawColumn(oldCursorPos)) {
					glColor3f(0, 0, 0);
					drawColumnColor(screenColumn(oldCursorPos));
				}
			}
			for (int i = cursorPos; i < firstVisibleColumn + visibleColumnsCnt; i++) {
				drawColumn(i);
				if ((++cnt % 16 == 0) && (glutGet(GLUT_ELAPSED_TIME)-time > MAX_PAINT_TIME)) {
					repaintInterrupted = true;
					break;
				}
			}
			if (!repaintInterrupted) {
				for (int i = cursorPos-1; i >= firstColumn; i--) {
					drawColumn(i);
					if ((++cnt % 16 == 0) && (glutGet(GLUT_ELAPSED_TIME)-time > MAX_PAINT_TIME)) {
						repaintInterrupted = true;
						break;
					}
				}
			}

			if (msgOption_showCursor && (screenCursorPos > 0) && (screenCursorPos < visibleColumnsCnt) &&
					playerPlaying || ((playerPosSec > 0) && (playerPosSec < playerDuration))) {
				glColor4fv(cursorColor);
				drawColumnColor(screenColumn(cursorPos));
				cursorDrawn = true;
			}
			tmTaskLeave(&drawerMainTask);
		} else {
			if (force) {
				repaintAllColumns = true;
			} else {
				copyRect(0, consoleHeight, width, height);
			}
			drawConsole(force);
		}
	} else {
		drawConsole(force);
	}
	glFlush();
	glutSwapBuffers();
}
