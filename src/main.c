// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <GL/freeglut.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "player.h"
#include "drawer.h"
//#include "fft.h"

// #define BENCHMARKS_ALLOWED

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
		float *newBuffer=malloc(sizeof(float)*blockLen*width);
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
	double minFreq=0, maxFreq=0;
	double anchoredFreq=440;
	double outputRate;
	outputRate=250;

	unsigned fftBlockLenLog2 = 0;
	unsigned fftShiftPerBlock = 0;
	unsigned fftOutputFromIndex = 0;
	unsigned fftOutputToIndex = 0;
	unsigned drawToIndex=0;
	unsigned fftThreads=0;
	bool fftNormalizeAmplitudes=true;
	bool toneScale=false;
	bool hideScaleLines=false;
	bool showKeyboard=false;
	bool coloredOvertones=false;

	bool quiet=false;
	bool fullscreen=false;

#define ifarg(option) if ((strcmp((option),argv[i])==0) && (i++,1))
#define tryarg(var,format) ((sscanf(argv[i], format, &var)==1) && (i++,1))
#define arg(option,var,format) \
	if (i>=argc) {\
		printf("Missing parameter of " option ".\n");\
		exit(1);\
	} else if (!tryarg(var, format)) {\
		printf("Wrong " option " parameter %s.\n", argv[i]);\
		exit(1);\
	}
#define arguint(option,var) arg(option,var,"%u")
#define argdbl(option,var) arg(option,var,"%lf")
	int i=1;
	while (i<argc) {
		ifarg("-?") {
			printf(
				"Command line args:\n"
				"\n"
				"  -?\n"
				"\n"
				"  --rate <sample_rate>\n"
				"\n"
				"  --scale [hz|tones] [<frequency>]\n"
				"       frequency: to be anchored / of a1\n"
				"       default: log hz 440\n"
				"  --hide-scale-lines\n"
				"  --show-keyboard\n"
				"\n"
				"  --colored-overtones\n"
				"\n"
				"  --frequency-range <minimum_frequency> <maximum_frequency>\n"
				"\n"
				"  --window-len-log2 <log2_of_samples_per_window>\n"
				"  --window-len <samples_per_window>\n"
				"       will be rounded to lower power of 2\n"
				"\n"
				"  --window-shift <samples_shift_per_window>\n"
				"  --window-shift-log2 <log2_of_samples_shift_per_window>\n"
				"  --output-rate <output-vectors-per-second>\n"
				"       default: 250\n"
				"\n"
				"  --normalization (window-len|volume)\n"
				"       window-len: amplitudes divided by window length\n"
				"       volume: highest amplitude equals volume (default)\n"
				"\n"
				"  --quiet\n"
				"  --fullscreen\n"
				"       warning: application responds only to window manager events\n"
				"  --exit-on-eof\n"
				"  --fft-threads <number>\n"
				"\n"
				"\n"
				"The application responds only to resizing or closing the window\n"
				"and to a new data on stdin.\n"
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
		} else ifarg("--rate") {
			argdbl("--rate",sampleRate);
		} else ifarg("--frequency-range") {
			argdbl("--frequency-range",minFreq);
			argdbl("--frequency-range",maxFreq);
		} else ifarg("--window-len") {
			arguint("--window-len", fftBlockLenLog2);
			fftBlockLenLog2=(unsigned)log2(fftBlockLenLog2);
		} else ifarg("--window-len-log2") {
			arguint("--window-len-log2", fftBlockLenLog2);
		} else ifarg("--window-shift") {
			arguint("--window-shift", fftShiftPerBlock);
			outputRate=0;
		} else ifarg("--window-shift-log2") {
			arguint("--window-len", fftShiftPerBlock);
			fftShiftPerBlock=1<<fftShiftPerBlock;
			outputRate=0;
		} else ifarg("--output-rate") {
			argdbl("--output-rate",outputRate);
			fftShiftPerBlock=0;
		} else ifarg("--normalization") {
			if (i>=argc) {
				printf("Missing parameter of --normalization.\n");
				exit(0);
			}
			ifarg("window-len") {
				fftNormalizeAmplitudes=false;
			} else ifarg("volume") {
				fftNormalizeAmplitudes=true;
			} else {
				printf("Wrong parameter of --normalization %s.\n", argv[i]);
				exit(0);
			}
		} else ifarg("--scale") {
			if (i>=argc) break;
			ifarg("hz") {
				toneScale=false;
			} else ifarg("tones") {
				toneScale=true;
			}
			if (i>=argc) break;
			double value;
			if (tryarg(value,"%lf")) {
				anchoredFreq=value;
			}
		} else ifarg("--quiet") {
			quiet=true;
		} else ifarg("--fullscreen") {
			fullscreen=true;
		} else ifarg("--exit-on-eof") {
			exitOnEOF=true;
		} else ifarg("--hide-scale-lines") {
			hideScaleLines=true;
		} else ifarg("--show-keyboard") {
			showKeyboard=true;
			toneScale=true;
		} else ifarg("--colored-overtones") {
			coloredOvertones=true;
		} else ifarg("--fft-threads") {
			arguint("--fft-threads", fftThreads);
		} else {
			printf("Unknown argument %s, use -? for help.\n", argv[i]);
			exit(1);
		}
	}
#undef argdbl
#undef arguint
#undef arg
#undef tryarg
#undef ifarg

	// check values
	if (sampleRate<0) {
		printf("Wrong sampling rate.\n");
		exit(1);
	}
	if (((maxFreq<=minFreq) && (minFreq!=0)) || (minFreq<0)) {
		printf("Wrong frequency range.\n");
		exit(1);
	}

	if (!fftBlockLenLog2) {
		if (minFreq>0) {
			fftBlockLenLog2=(unsigned)log2(sampleRate/minFreq);
			double displayedHeightPow2=log2(maxFreq/minFreq);
			if (displayedHeightPow2<9)
				fftBlockLenLog2+=9-(unsigned)displayedHeightPow2;
		} else {
			fftBlockLenLog2=13;
		}
	}
	if (!fftShiftPerBlock) {
		if (outputRate==0) {
			printf("Wrong output rate / window shift.");
			exit(1);
		}
		if (sampleRate)
			fftShiftPerBlock = sampleRate/outputRate;
		else
			fftShiftPerBlock = 1<<(fftBlockLenLog2-1);
		if (!fftShiftPerBlock)
			fftShiftPerBlock=1;
	} else {
		outputRate=sampleRate/fftShiftPerBlock;
	}

	if (minFreq>0) {
		fftOutputFromIndex=(1<<fftBlockLenLog2)*minFreq/sampleRate;
	}
	if (fftOutputFromIndex<=0) {
		fftOutputFromIndex=1;
	}
	if (maxFreq>0) {
		drawToIndex=(1<<fftBlockLenLog2)*maxFreq/sampleRate;
		if (drawToIndex>=(1<<fftBlockLenLog2))
			drawToIndex = 1<<fftBlockLenLog2;
	} else {
		drawToIndex = 1<<(fftBlockLenLog2-1);
	}
	if (fftOutputFromIndex>drawToIndex) {
		fftOutputFromIndex=0;
	}
	if (drawToIndex>(1<<(fftBlockLenLog2-1)))
		printf("Warning: too high frequency displayed, graph will be vertically mirrored\n");

	minFreq=(double)sampleRate*fftOutputFromIndex/(1<<fftBlockLenLog2);
	maxFreq=(double)sampleRate*drawToIndex/(1<<fftBlockLenLog2);

	if (!fftThreads)
		fftThreads=sysconf(_SC_NPROCESSORS_ONLN);

	// print settings
	if (!quiet) {
		printf("Assumed sampling rate:           ");
		if (sampleRate>0)
			printf("%.2lf Hz\n", sampleRate);
		else
			printf("none\n");

		printf("Displayed frequency range:       ");
		if (maxFreq>0)
			printf("%.2lf Hz - %.2lf Hz\n", minFreq, maxFreq);
		else
			printf("unknown\n");

		printf("Output rate:                     %0.2lf Hz (window is shifting by %d samples)\n",
			(double)sampleRate/fftShiftPerBlock, fftShiftPerBlock);

		printf("Scale:                           ");
		if (toneScale)
			printf("tone labels, a1 frequency %.2lf\n", anchoredFreq);
		else
			printf("hz, anchored frequency %.2lf\n", anchoredFreq);

		printf("Window length:                   %u samples (2^%u), displayed %d-%d\n",
			1<<fftBlockLenLog2, fftBlockLenLog2, fftOutputFromIndex, drawToIndex);

		printf("Amplitude normalization:         ");
		if (fftNormalizeAmplitudes)
			printf("by volume\n");
		else
			printf("by window length\n");

		printf("FFT threads: %u\n", fftThreads);
	}
	if (coloredOvertones)
		fftOutputToIndex=(1<<fftBlockLenLog2)-1;
	else
		fftOutputToIndex=drawToIndex;
	showKeyboard=showKeyboard && toneScale;
	

	/*
	fftStart(stdin,
		fftBlockLenLog2, fftShiftPerBlock,
		fftOutputFromIndex, fftOutputToIndex,
		fftNormalizeAmplitudes, fftThreads);

	blockLen=fftOutputToIndex-fftOutputFromIndex+1;
	blocksAlloc=16;
	buffer=malloc(sizeof(float)*blockLen*blocksAlloc);

	drawerSetInputColumnLen(blockLen,drawToIndex-fftOutputFromIndex+1);
	*/


	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE);
	glutCreateWindow("Music analyser"); // XXX

	playerUseFDQuiet(stdin, sampleRate);
	drawerInit(100, 1, minFreq, maxFreq, anchoredFreq, toneScale, hideScaleLines, showKeyboard, coloredOvertones); // XXX

	if (fullscreen)
		glutFullScreen();

	glutMainLoop();
	return 0;
}

