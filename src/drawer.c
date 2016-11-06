// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include<stdlib.h>
#include<stdbool.h>
#include<math.h>
#include<stdio.h>

#include<GL/freeglut.h>
#include<GL/gl.h>

#include "player.h"
#include "drawerBuffer.h"
#include "drawerMusicVisualiser.h"
#include "drawerScale.h"
#include "drawer.h"

static size_t width, height, rowSize;
static size_t rowBegin=0;
static size_t newColumns=0;
static size_t inputColumnLen=1;
static size_t inputColumnLenToDraw=1;

bool drawScaleLines=true;
bool drawKeyboard=false;
bool colorOvertones=false;
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

static void onTimer();
static void onReshape(int w, int h);
static void onDisplay();

void drawerInit(
	double columnsPerSecond, double centeredColumnRatio,
	double minFrequency, double maxFrequency, double anchoredFrequency,
	bool tones, bool hideScaleLines, bool showKeyboard, bool coloredOvertones) {
	dbufferColumnsPerSecond = columnsPerSecond;
	dsColumnToPlayerPosMultiplier = playerFreqRate/columnsPerSecond;
	centeredRatio = centeredColumnRatio;
	dsMinFreq=minFrequency;
	dsMaxFreq=maxFrequency;
	dsAnchoredFreq=anchoredFrequency;
	dsTones=tones;
	drawScaleLines=!hideScaleLines;
	drawKeyboard=showKeyboard;
	colorOvertones=coloredOvertones;
	//calcScaleLabels();

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

static inline void drawColumnColors(int screenColumn, unsigned char *colors) {
	glRasterPos2i(screenColumn,0);
	glDrawPixels(1, height, GL_RGB, GL_UNSIGNED_BYTE, colors);
}
static inline void drawColumnColor(int screenColumn, GLbyte *color) {
	glColor3ubv(color);
	glLineWidth(1);
	glBegin(GL_LINES);
	glVertex2i(screenColumn, 0);
	glVertex2i(screenColumn, height);
	glEnd();
}
static inline void drawColumn(int screenColumn, bool previewOnly) {
	int column = screenColumn - screenCenterColumn + drawerBufferPos;
	char *colors=NULL;
	if (dbufferExists(column)) {
		dbufferDrawn(column) = true;
		__sync_synchronize(); // XXX maybe slow
		if (!previewOnly && (dbufferPrecision(column) > 0)) {
			colors=&dbuffer(column, 0, 0);
			drawColumnColors(screenColumn, colors);
			return;
		} else {
			if (!dbufferPreviewCreated(column)) {
				dmvCreatePreview(column);
			}
			if (dbufferPreviewCreated(column)) {
				drawColumnColor(screenColumn, &dbufferPreview(column, 0));
			} else {
				drawColumnColor(screenColumn, (unsigned char []){255, 0, 0});
			}
			if (previewOnly && (dbufferPrecision(column) > 0)) {
				dbufferDrawn(column) = false;
			}
		}
	} else {
		drawColumnColor(screenColumn, (unsigned char []){0, 255, 255});
	}
}
static inline void moveColumns(int offset) {
	int cnt, from, to;
	if (offset > 0) {
		cnt  = width - offset; from = offset; to = 0;
	} else {
		cnt  = width + offset; from = 0; to = -offset;
	}
	glRasterPos2i(to, 0);
	glCopyPixels(from, 0, cnt, height, GL_COLOR);
}

// Draws the view, possibly moving some area (if not forced)
static void repaint(bool force) {
	static int oldBufferPos=0;
	int offset=drawerBufferPos-oldBufferPos;
	if (offset > 0) {
		for (int i = width-offset+oldBufferPos-screenCenterColumn; i<width+oldBufferPos-screenCenterColumn; i++) {
			dbufferDrawn(i) = false;
		}
	} else {
		for (int i = oldBufferPos-screenCenterColumn; i<offset+oldBufferPos-screenCenterColumn; i++) {
			dbufferDrawn(i) = false;
		}
	}
	if (!force && (abs(offset) < width)) {
		moveColumns(offset);
	}

	for (int i=0; i<width; i++) {
		if (!dbufferDrawn(i+drawerBufferPos-screenCenterColumn)) {
			drawColumn(i, force);
		}
	}

	oldBufferPos = drawerBufferPos;


	/*
	static int leftForcedColumns=0;
	if (!force && !newColumns)
		return;

	//glClear(GL_COLOR_BUFFER_BIT);
	int x=0;
	glRasterPos2i(0,0);
	if (!force && (newColumns<width)) {
		// redraw forced left columns
		leftForcedColumns-=newColumns;
		if (leftForcedColumns>0) {
			glPixelStoref(GL_UNPACK_SKIP_PIXELS, rowBegin);
			if (width-rowBegin>=leftForcedColumns) {
				glDrawPixels(leftForcedColumns, height, GL_RGB, GL_UNSIGNED_BYTE, buffer);
			} else {
				glDrawPixels(width-rowBegin, height, GL_RGB, GL_UNSIGNED_BYTE, buffer);
				glRasterPos2i(width-rowBegin, 0);
				glPixelStoref(GL_UNPACK_SKIP_PIXELS, 0);
				glDrawPixels(leftForcedColumns-(width-rowBegin), height, GL_RGB, GL_UNSIGNED_BYTE, buffer);
			}
			x=leftForcedColumns;
			glRasterPos2i(x,0);
			leftForcedColumns=0;
		}

		// move valid rectangle in graphic card
		glCopyPixels(newColumns+x,0,width-newColumns-x,height,GL_COLOR);
		x=width-newColumns;
		glRasterPos2i(x,0);
	}

	if (rowBegin+x<width) {
		// draw necessary part from rowBegin to end of buffer
		glPixelStoref(GL_UNPACK_SKIP_PIXELS,rowBegin+x);
		glDrawPixels(width-rowBegin-x,height,GL_RGB,GL_UNSIGNED_BYTE,buffer);
		glRasterPos2i(width-rowBegin,0);
		x=0;
	} else
		x-=width-rowBegin;

	// draw necessary part from the begining of buffer to rowBegin
	glPixelStoref(GL_UNPACK_SKIP_PIXELS,x);
	glDrawPixels(rowBegin-x,height,GL_RGB,GL_UNSIGNED_BYTE,buffer);


	// draw labels
	glColor3f(0.0f,1.0f,0.0f);
	GLint rasterPos[4];
	for (size_t i=0; i<scaleLabelsCnt; i++) {
		glRasterPos2i(5, scaleLabels[i].pos-3*!drawScaleLines);
		glutBitmapString(SCALE_LABELS_FONT, (unsigned char*)scaleLabels[i].label);
		glGetIntegerv(GL_CURRENT_RASTER_POSITION, rasterPos);
		if (leftForcedColumns<rasterPos[0]) {
			leftForcedColumns=rasterPos[0];
		}
	}
	*/

	glFlush();
	glutSwapBuffers();
}

// --- EVENTS ---

static void onTimer() {
	__sync_synchronize();
	int newPos=dsPlayerPosToColumn(playerPos);
	if (drawerBufferPos != newPos) {
		dbufferMove(newPos-drawerBufferPos);
		drawerBufferPos = newPos;
		dmvRefresh();
		repaint(false);
	}
	glutTimerFunc(DRAWER_FRAME_DELAY, onTimer, 0);
}

static void onReshape(int w, int h) {
	width=w;
	height=h;
	screenCenterColumn=w*centeredRatio;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, 0, h, -1, 1);
	glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glTranslatef(0.375, 0.375, 0.);
	dmvResize(h, screenCenterColumn, w-screenCenterColumn);

	glReadBuffer(GL_FRONT);
	glDrawBuffer(GL_BACK);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//glPixelStoref(GL_UNPACK_ROW_LENGTH,width);
	//glClear(GL_COLOR_BUFFER_BIT);
}

static void onDisplay() {
	repaint(true);
}
