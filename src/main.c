// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <GL/freeglut.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "util.h"
#include <unistd.h>

#include "player.h"
#include "drawer.h"

bool exitOnEOF=false;

#define FRAME_DELAY 10 // ms
int width=0, height;

float *buffer;
size_t blockLen, blocksAlloc;
size_t firstBlock=0, blocksCnt=0;
#define buffer(block,index) buffer[(block)*blockLen+(index)]

void exitApp() {
	exit(0);
}

/*
bool resized=false;
void onResize(int w, int h){
	width=w;
	height=h;
	size_t newBlocksAlloc=width;
	if (newBlocksAlloc*blockLen*sizeof(float)>256*1024*1024)  // max 256MB buffer
		newBlocksAlloc=256*1024*1024/sizeof(float)/blockLen;    // some history will be forgotten when resized
	if (blocksAlloc<newBlocksAlloc) {
		float *newBuffer=utilMalloc(sizeof(float)*blockLen*width);
		if (newBuffer) {
			if (firstBlock+blocksCnt<=blocksAlloc) {
				memcpy(newBuffer, &buffer(firstBlock,0), sizeof(float)*blocksCnt*blockLen);
			} else {
				memcpy(newBuffer, &buffer(firstBlock,0), sizeof(float)*(blocksAlloc-firstBlock)*blockLen);
				memcpy(newBuffer+(blocksAlloc-firstBlock)*blockLen, buffer, sizeof(float)*(blocksCnt-(blocksAlloc-firstBlock))*blockLen);
			}
			free(buffer);
			buffer=newBuffer;
			firstBlock=0;
			blocksAlloc=newBlocksAlloc;
		}
	}
	resized=true;
}
*/


/*
void onTimer(int nothing) {
	static bool eof=false;
		glutTimerFunc(FRAME_DELAY, onTimer, 0);
	if (eof) {
		if (exitOnEOF)
			exitApp();
	}
	size_t cnt;
	while (true) {
		size_t firstNewBlock=(firstBlock+blocksCnt)%blocksAlloc;
		cnt=fftGetData(&buffer(firstNewBlock,0), blocksAlloc-firstNewBlock);
		if (fftEof()) {
			eof=true;
		}
		if (!cnt)
			break;
			drawerAddColumns(&buffer(firstNewBlock,0), cnt, 0);
		blocksCnt+=cnt;
		if (blocksCnt>blocksAlloc) {
			firstBlock=(firstBlock+blocksCnt-blocksAlloc)%blocksAlloc;
			blocksCnt=blocksAlloc;
		}
	}
	if (resized) {
		resized=false;
		glViewport(0,0,width,height);
		glLoadIdentity();
		glOrtho(0,width,0,height,-1,1);
		drawerClearBuffer(width,height);
		if (firstBlock+blocksCnt<=blocksAlloc) {
			drawerAddColumns(&buffer(firstBlock,0), blocksCnt, 0);
		} else {
			drawerAddColumns(&buffer(firstBlock,0), blocksAlloc-firstBlock, 0);
			drawerAddColumns(buffer, blocksCnt-(blocksAlloc-firstBlock), 0);
		}
		drawerRepaint(true);
	} else {
		drawerRepaint(false);
	}

}
*/



int main(int argc, char **argv){
	double sampleRate=0;
	double minFreq=16, maxFreq=22050;
	double a1Freq=440;
	double outputRate=100;
	float unplayedPerc = 0;

	char c;
	bool showHelp=false;
	while (!showHelp && ((c=getopt(argc, argv, "?i:r:c:u:a:")) != -1)) {
		switch (c) {
			case '?':
				showHelp=true;
				break;
			case 'i':
				sampleRate=strtof(optarg, &optarg);
				if (*optarg) showHelp=true;
				break;
			case 'r':
				if (sscanf(optarg, "%lf-%lf", &minFreq, &maxFreq, &c) != 2) showHelp=true;
				break;
			case 'a':
				if (sscanf(optarg, "%lf", &a1Freq, &c) != 1) showHelp=true;
				break;
			case 'c':
				if (sscanf(optarg, "%lf", &outputRate, &c) != 1) showHelp=true;
				break;
			case 'u':
				if (sscanf(optarg, "%f", &unplayedPerc, &c) != 1) showHelp=true;
				break;
		}
	}
	if (showHelp) {
		printf(
			"\n"
			"Help will be written later.\n" // XXX
			"\n\n"
			"This program is free software: you can redistribute it and/or modify\n"
			"it under the terms of the GNU General Public License version 3\n"
			"as published by the Free Software Foundation.\n"
			"\n"
			"This program is distributed in the hope that it will be useful,\n"
			"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
			"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
			"GNU General Public License for more details.\n"
			"\n"
			"You should have received a copy of the GNU General Public License\n"
			"along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
			"\n"
			"Copyright (C) 2015  Lukáš Ondráček <ondracek.lukas@gmail.com>.\n"
			"\n");
		exit(0);
	}

	// check values
	if (outputRate<0) {
		printf("Wrong output rate.\n");
		exit(1);
	}
	if (sampleRate<0) {
		printf("Wrong sampling rate.\n");
		exit(1);
	}
	if ((maxFreq<=minFreq) || (minFreq<=0)) {
		printf("Wrong frequency range.\n");
		exit(1);
	}
	if ((unplayedPerc>100) || (unplayedPerc<0)) {
		printf("Wrong unplayed ratio.\n");
		exit(1);
	}


	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE);
	glutCreateWindow("MusA (musical analyser)");

	playerUseFDQuiet(stdin, sampleRate);
	drawerInit(outputRate, unplayedPerc, minFreq, maxFreq, a1Freq);

	glutMainLoop();
	return 0;
}

