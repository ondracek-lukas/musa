// Spectrograph  Copyright (C) 2015  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include<stdlib.h>
#include<stdbool.h>
#include<math.h>
#include<stdio.h>

#include<GL/freeglut.h>
#include<GL/gl.h>

#include "drawer.h"

static size_t width, height, rowSize;
static size_t rowBegin=0;
static size_t newColumns=0;
static size_t inputColumnLen=1;
static size_t inputColumnLenToDraw=1;

static enum drawerScale scale=UNSCALED;
static double scaleMinFreq, scaleMaxFreq, scaleAnchoredFreq;
bool scaleTones=false;
bool drawScaleLines=true;
bool drawKeyboard=false;
bool colorOvertones=false;
float keyboardMinFreq;
unsigned keyboardMinTone;

static unsigned char *buffer=0;
#define buffer(x,y,channel) buffer[rowSize*(y)+3*(((x)+rowBegin)%width)+channel]

#define SCALE_LINES_ALPHA_MAIN 0.5
#define SCALE_LINES_ALPHA_OTHER 0.3
#define MIN_SCALE_LABELS_DIST 50 // px
#define SCALE_LABELS_FONT_HEIGHT 15 // px
#define SCALE_LABELS_FONT_HEIGHT_UP 12 // px
#define SCALE_LABELS_FONT GLUT_BITMAP_HELVETICA_12
#define KEYBOARD_ALPHA 0.5

static unsigned char colorScale[1024*3]={255};


struct scaleLabels {
	size_t pos;
	bool main;
	char label[32];
} *scaleLabels=NULL;
size_t scaleLabelsCnt=0;


static double scaleFreqToPosMultiplier;
static inline double scaleFreqToPos(double freq) {
	switch (scale) {
		case LOG_SCALE:
			// scaleFreqToPosMultiplier=(double)height/log(scaleMaxFreq/scaleMinFreq);
			return log(freq/scaleMinFreq)*scaleFreqToPosMultiplier;
		case LINEAR_SCALE:
			// scaleFreqToPosMultiplier=(double)height/(scaleMaxFreq-scaleMinFreq);
			return (freq-scaleMinFreq)*scaleFreqToPosMultiplier;
		default:
			return 0;
	}
}
static double scalePosToInputIndexMultiplier;
static double *scalePosToInputIndexCache=0;
#define scalePosToInputIndex(pos) scalePosToInputIndexCache[pos]
static inline double scalePosToInputIndexUncached(double pos) {
	switch (scale) {
		case LOG_SCALE:
			// scalePosToInputIndexMultiplier=scaleMinFreq/(scaleMaxFreq-scaleMinFreq)*(inputColumnLenToDraw-1);
			return (pow(scaleMaxFreq/scaleMinFreq,pos/height)-1)*scalePosToInputIndexMultiplier;
		case LINEAR_SCALE:
			// scalePosToInputIndexMultiplier=(double)(inputColumnLenToDraw-1)/height;
			return pos*scalePosToInputIndexMultiplier;
		default:
			return 0;
	}
}

static inline double scaleFreqToInputIndex(double freq) {
	return (freq-scaleMinFreq)/(scaleMaxFreq-scaleMinFreq)*(inputColumnLenToDraw-1);
}
static inline double scaleInputIndexToFreq(double index) {
	return index/(inputColumnLenToDraw-1)*(scaleMaxFreq-scaleMinFreq)+scaleMinFreq;
}

static void getToneName(char *name, int a1offset) {
	int octave, tone=a1offset+21; // c offset
	char *toneName;
	if (tone>=0) {
		octave=tone/12;
		tone%=12;
		toneName=(char *[]){"c","cis","d","dis","e","f","fis","g","gis","a","ais","h"}[tone];
		if (octave>0)
			sprintf(name, "%s%d", toneName, octave);
		else
			sprintf(name, "%s", toneName);
	} else {
		octave=(-tone-1)/12;
		tone=((tone%12)+12)%12;
		toneName=(char *[]){"C","Cis","D","Dis","E","F","Fis","G","Gis","A","Ais","H"}[tone];
		if (octave>0)
			sprintf(name, "%s%d", toneName, octave);
		else
			sprintf(name, "%s", toneName);
	}
}

static void calcScaleLabels() {
	if (!height)
		return;

	// initialize scale
	switch (scale) {
		case LOG_SCALE:
			scaleFreqToPosMultiplier=(double)height/log(scaleMaxFreq/scaleMinFreq);
			scalePosToInputIndexMultiplier=scaleMinFreq/(scaleMaxFreq-scaleMinFreq)*(inputColumnLenToDraw-1);
			break;
		case LINEAR_SCALE:
			scaleFreqToPosMultiplier=(double)height/(scaleMaxFreq-scaleMinFreq);
			scalePosToInputIndexMultiplier=(double)(inputColumnLenToDraw-1)/height;
			break;
		default:
			break;
	}
	free(scalePosToInputIndexCache);
	scalePosToInputIndexCache=malloc(sizeof(double)*height);
	for (size_t i=0; i<height; i++)
		scalePosToInputIndexCache[i]=scalePosToInputIndexUncached(i);

	float multiplier, difference, freq;
	double freq1Pos;
	freq=scaleAnchoredFreq;
	if (scale && scaleTones) {
		multiplier=pow(2,1.0/12);
		int tone=0, lastLabeledPos=-SCALE_LABELS_FONT_HEIGHT;
		for (; freq>=scaleMinFreq; freq/=multiplier, tone--);
		for (; freq<scaleMinFreq; freq*=multiplier, tone++);
		keyboardMinFreq=freq/pow(2,1.0/24);
		keyboardMinTone=((tone-4)%12+12)%12;
		free(scaleLabels);
		scaleLabelsCnt=(size_t)ceil(log(scaleMaxFreq/freq)/log(multiplier));
		scaleLabels=malloc(sizeof(struct scaleLabels)*scaleLabelsCnt);
		for (size_t i=0; i<scaleLabelsCnt; freq*=multiplier, i++, tone++) {
			scaleLabels[i].pos=(size_t)(scaleFreqToPos(freq)+0.5);
			if ((height-scaleLabels[i].pos>SCALE_LABELS_FONT_HEIGHT) &&
			    (scaleLabels[i].pos-lastLabeledPos>SCALE_LABELS_FONT_HEIGHT)) {
				getToneName(scaleLabels[i].label, tone);
				scaleLabels[i].main=true;
				lastLabeledPos=scaleLabels[i].pos;
			} else {
				scaleLabels[i].label[0]='\0';
				scaleLabels[i].main=false;
			}
		}
	} else {
		switch (scale) {
			case LOG_SCALE:
				multiplier=2;
				freq1Pos=scaleFreqToPos(1);
				for (; scaleFreqToPos(multiplier)-freq1Pos>=MIN_SCALE_LABELS_DIST; multiplier=sqrt(multiplier));
				for (; scaleFreqToPos(multiplier)-freq1Pos<MIN_SCALE_LABELS_DIST; multiplier=pow(multiplier,2));
				for (; freq>=scaleMinFreq; freq/=multiplier);
				for (; freq<scaleMinFreq; freq*=multiplier);
				free(scaleLabels);
				scaleLabelsCnt=(size_t)ceil(log(scaleMaxFreq/freq)/log(multiplier));
				scaleLabels=malloc(sizeof(struct scaleLabels)*scaleLabelsCnt);
				for (size_t i=0; i<scaleLabelsCnt; freq*=multiplier, i++) {
					scaleLabels[i].pos=(size_t)(scaleFreqToPos(freq)+0.5);
					if (height-scaleLabels[i].pos>SCALE_LABELS_FONT_HEIGHT)
						sprintf(scaleLabels[i].label,"%0.2f Hz", freq);
					else
						scaleLabels[i].label[0]='\0';
					scaleLabels[i].main=true;
				}
				break;
			case LINEAR_SCALE:
				difference=10;
				freq1Pos=scaleFreqToPos(0);
				for (; scaleFreqToPos(difference)-freq1Pos>=MIN_SCALE_LABELS_DIST; difference/=2);
				for (; scaleFreqToPos(difference)-freq1Pos<MIN_SCALE_LABELS_DIST; difference*=2);
				for (; freq>=scaleMinFreq; freq-=difference);
				for (; freq<scaleMinFreq; freq+=difference);
				free(scaleLabels);
				scaleLabelsCnt=(size_t)ceil((scaleMaxFreq-freq)/difference);
				scaleLabels=malloc(sizeof(struct scaleLabels)*scaleLabelsCnt);
				for (size_t i=0; i<scaleLabelsCnt; freq+=difference, i++) {
					scaleLabels[i].pos=(size_t)(scaleFreqToPos(freq)+0.5);
					if (height-scaleLabels[i].pos>SCALE_LABELS_FONT_HEIGHT)
						sprintf(scaleLabels[i].label,"%0.2f Hz", freq);
					else
						scaleLabels[i].label[0]='\0';
					scaleLabels[i].main=true;
				}
				break;
			default:
				free(scaleLabels);
				scaleLabels=NULL;
				scaleLabelsCnt=0;
				break;
		}
	}
}

void drawerSetScale(enum drawerScale s, double minFrequency, double maxFrequency, double anchoredFrequency,
	bool tones, bool hideScaleLines, bool showKeyboard, bool coloredOvertones) {
	scale=s;
	scaleMinFreq=minFrequency;
	scaleMaxFreq=maxFrequency;
	scaleAnchoredFreq=anchoredFrequency;
	scaleTones=tones;
	drawScaleLines=!hideScaleLines;
	drawKeyboard=showKeyboard;
	colorOvertones=coloredOvertones;
	if (scale)
		calcScaleLabels();
	else
		scale=UNSCALED;
}

void drawerSetInputColumnLen(size_t value, size_t toBeDrawen) {
	inputColumnLen=value;
	inputColumnLenToDraw=toBeDrawen;
}

void drawerAddColumns(float *buff, size_t columns, size_t drawingOffset) { // column-oriented
	if (!buffer)
		return;
	for (size_t x=0; x<columns; x++) {
		float avg=0;
		bool keyboardWhite;
		double keyboardFreq=keyboardMinFreq;
		unsigned keyboardTone=keyboardMinTone;
		for (size_t i=0; i<inputColumnLen; i++)
			avg+=buff[x*inputColumnLen+i];
		avg/=inputColumnLen;
		for (size_t y=0; y<height; y++) {
			float value;
			double freq;
			if (scale) {
				double dblIndex=scalePosToInputIndex(y);
				double ratio=dblIndex;
				freq=scaleInputIndexToFreq(dblIndex);
				size_t index=(int)ratio;
				ratio-=index;
				value=buff[x*inputColumnLen+index]*(1-ratio) + buff[x*inputColumnLen+index+1]*ratio; // ?
			} else {
				value=buff[x*inputColumnLen+y];
			}
			if (scale && colorOvertones) {
				float odd=1, even=1, none=1, coef;
				unsigned tmp;
				if (scaleFreqToInputIndex(freq*2)<=inputColumnLen)
					odd+= (buff[x*inputColumnLen+(int)(scaleFreqToInputIndex(freq*2))]-avg)/value*4;
				if (scaleFreqToInputIndex(freq*3)<=inputColumnLen)
					even+=(buff[x*inputColumnLen+(int)(scaleFreqToInputIndex(freq*3))]-avg)/value*4;
				if (scaleFreqToInputIndex(freq*4)<=inputColumnLen)
					odd+= (buff[x*inputColumnLen+(int)(scaleFreqToInputIndex(freq*4))]-avg)/value*2;
				if (scaleFreqToInputIndex(freq*5)<=inputColumnLen)
					even+=(buff[x*inputColumnLen+(int)(scaleFreqToInputIndex(freq*5))]-avg)/value*2;
				if (scaleFreqToInputIndex(freq*6)<=inputColumnLen)
					odd+= (buff[x*inputColumnLen+(int)(scaleFreqToInputIndex(freq*6))]-avg)/value;
				if (scaleFreqToInputIndex(freq*7)<=inputColumnLen)
					even+=(buff[x*inputColumnLen+(int)(scaleFreqToInputIndex(freq*7))]-avg)/value;
				if (odd<1) odd=1;
				if (even<1) even=1;
				coef=1/(odd+even+1)*256*3;

				if ((tmp=value*even*coef)<=255)
					buffer(x+drawingOffset,y,0)=tmp;
				else
					buffer(x+drawingOffset,y,0)=255;
				if ((tmp=value*none*coef)<=255)
					buffer(x+drawingOffset,y,1)=tmp;
				else
					buffer(x+drawingOffset,y,1)=255;
				if ((tmp=value*odd*coef)<=255)
					buffer(x+drawingOffset,y,2)=tmp;
				else
					buffer(x+drawingOffset,y,2)=255;

			} else {
				unsigned valueI=value*1024;
				if (valueI>=1024)
					valueI=1023;
				valueI*=3;
				buffer(x+drawingOffset,y,0)=colorScale[valueI];
				buffer(x+drawingOffset,y,1)=colorScale[valueI+1];
				buffer(x+drawingOffset,y,2)=colorScale[valueI+2];
			}
			if (drawKeyboard) {
				if (keyboardFreq>freq) {
					buffer(x+drawingOffset,y,0)=(1-KEYBOARD_ALPHA)*buffer(x+drawingOffset,y,0)+KEYBOARD_ALPHA*(keyboardWhite*128+!keyboardTone*127);
					buffer(x+drawingOffset,y,1)=(1-KEYBOARD_ALPHA)*buffer(x+drawingOffset,y,1)+KEYBOARD_ALPHA*(keyboardWhite*128+!keyboardTone*127);
					buffer(x+drawingOffset,y,2)=(1-KEYBOARD_ALPHA)*buffer(x+drawingOffset,y,2)+KEYBOARD_ALPHA*(keyboardWhite*128+!keyboardTone*127);
				} else {
					while (keyboardFreq<=freq) {
						keyboardFreq*=pow(2,1.0/12);
						keyboardTone=(keyboardTone+1)%12;
					}
					switch (keyboardTone) {
						case 1:
						case 3:
						case 6:
						case 8:
						case 10:
							keyboardWhite=false;
							break;
						default:
							keyboardWhite=true;
							break;
					}
				}
			}
		}
		if (drawScaleLines) {
			for (size_t i=0; i<scaleLabelsCnt; i++) {
				size_t y=scaleLabels[i].pos;
				float alpha=
					scaleLabels[i].main*SCALE_LINES_ALPHA_MAIN+
					(1-scaleLabels[i].main)*SCALE_LINES_ALPHA_OTHER;
				buffer(x+drawingOffset,y,0)=(1-alpha)*buffer(x+drawingOffset,y,0);
				buffer(x+drawingOffset,y,1)=(1-alpha)*buffer(x+drawingOffset,y,1)+alpha*255;
				buffer(x+drawingOffset,y,2)=(1-alpha)*buffer(x+drawingOffset,y,2);
			}
		}
	}
	rowBegin=(rowBegin+columns+drawingOffset+width) % width;
	newColumns+=columns;
}
void drawerClearBuffer(size_t w, size_t h){
	width=w;
	height=h;
	rowBegin=0;
	rowSize=(3*width+3)/4*4;
	free(buffer);
	buffer=calloc(height*rowSize,1);
	calcScaleLabels();
	glReadBuffer(GL_BACK);
	//glReadBuffer(GL_FRONT);
	glDrawBuffer(GL_BACK);
	glPixelStoref(GL_UNPACK_ROW_LENGTH,width);
	//newColumns=width; // ?
	glClear(GL_COLOR_BUFFER_BIT);
	if (!(scale && colorOvertones) && (colorScale[0]==255)) { // initialize colorScale cache
		size_t i=0;
		for (; i<64; i++) {
			colorScale[3*i]  =0;
			colorScale[3*i+1]=0;
			colorScale[3*i+2]=sqrt(i)*16;
		}
		for (; i<144; i++) {
			colorScale[3*i]  =sqrt(i)*32-256;
			colorScale[3*i+1]=0;
			colorScale[3*i+2]=128;
		}
		for (; i<256; i++) {
			colorScale[3*i]  =sqrt(i)*32-256;
			colorScale[3*i+1]=0;
			colorScale[3*i+2]=511-sqrt(i)*32;
		}
		for (; i<1024; i++) {
			colorScale[3*i]  =255;
			colorScale[3*i+1]=sqrt(i)*16-256;
			colorScale[3*i+2]=0;
		}
	}
}


void drawerRepaint(bool force) {
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


	glFlush();
	glutSwapBuffers();
	newColumns=0;
}
