// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <math.h>
#include <float.h>
#include <limits.h>
#include <string.h>
#include "logFft.h"
#include "drawerScale.h"
#include "drawerBuffer.h"
#include "player.h"
#include "util.h"
#include "taskManager.h"
#include "messages.h"
#include "resampler.h"
#include "overtoneFilter.h"

#define MAX_PRECISION_LEVELS 32

static int
	 processedFrom[MAX_PRECISION_LEVELS],
	 processedTo[MAX_PRECISION_LEVELS];
static unsigned char minPrecision, maxPrecision;
static int fftBlockLen;


struct taskInfo dmvTask = TM_TASK_INITIALIZER(false, false);


// --- COLORING ---

static unsigned char colorScale[1024*3]={255}; // will be initialized below

static inline void toColorScale(float value, unsigned char *color) {
	unsigned valueI = value*1024;
	if (valueI>=1024)
		valueI=1023;
	valueI*=3;

	color[0] = colorScale[valueI];
	color[1] = colorScale[valueI+1];
	color[2] = colorScale[valueI+2];
}


static void createColumn(int column, float *logFftResult) { // assert: logFftResult can be modified
	if (msgOption_filterOvertones) {
		ofProcess(logFftResult);
	}

	double maxAmp = 0.0001;
	double minAmp = logFftResult[0];
	for (int i=0; i<dbuffer.columnLen; i++) {
		if (maxAmp < logFftResult[i]) maxAmp = logFftResult[i];
		if (minAmp > logFftResult[i]) minAmp = logFftResult[i];
	}


	for (int i=0; i<dbuffer.columnLen; i++) {
		double amp = logFftResult[i];

		if (msgOption_dynamicGain) {
			amp = log10(amp/maxAmp);
		} else {
			amp = log10(amp) + msgOption_gain*0.1;
		}
		amp = amp / (0.1*msgOption_signalToNoise) + 1;

		if      (amp > 1) amp = 1;
		else if (amp < 0) amp = 0;

		toColorScale(amp, &dbuffer(column, i, 0));
	}
}



// --- INIT FUNCTIONS ---

static bool taskFunc();
void dmvInit() {
	tmTaskRegister(&dmvTask, taskFunc, 1);

	// colorScale init
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


static void onRestart();

void dmvReset() { // pause dmvTask and drawerMainTask before
	if (playerSourceType == PLAYER_SOURCE_NONE) {
		dmvTask.active = false;
		return;
	}
	if ((msgOption_columnLen<=0) || (msgOption_minFreq > msgOption_maxFreq)) { // XXX check elsewhere
		dmvTask.active = false;
		return;
	}
	dbufferRealloc(msgOption_columnLen);
	int pos = dsPlayerPosToColumn(playerPos);
	for (int i=0; i<MAX_PRECISION_LEVELS; i++) {
		processedTo[i] = processedFrom[i] = pos;
	}

	logFftReset(
			msgOption_minFreq,
			msgOption_maxFreq,
			msgOption_columnLen, &fftBlockLen);
	minPrecision = 1;
	maxPrecision = rsCnt;
	dmvTask.active = true;
}

void dmvRefresh() {
	if (!tmTaskEnter(&dmvTask)) return;
	if ((processedTo[maxPrecision] < dbuffer.begin) || (processedFrom[maxPrecision] > dbuffer.end)) {
		int pos = dsPlayerPosToColumn(playerPos);
		for (unsigned char p = minPrecision; p <= maxPrecision; p++) {
			processedTo[p] = processedFrom[p] = pos;
		}
	} else {
		unsigned char p = maxPrecision;
		while (true) {
			unsigned char p2 = dbufferPrecision(processedTo[p]);
			if ((processedTo[p] >= dbuffer.end) || (dbufferState(processedTo[p]) < DBUFF_READY)) p2 = 0;
			while ((p > p2) && (p > minPrecision)) {
				if (processedTo[p-1] < processedTo[p]) {
					processedTo[p-1] = processedTo[p];
				}
				p--;
			}
			if (p2 < minPrecision) break;
			processedTo[p]++;
		}
		for (p = minPrecision; (p <= maxPrecision) && (processedTo[p] > dbuffer.end); p++) {
			processedTo[p] = dbuffer.end;
		}

		p = maxPrecision;
		while (true) {
			unsigned char p2 = dbufferPrecision(processedFrom[p]-1);
			if ((processedFrom[p] <= dbuffer.begin) || (dbufferState(processedFrom[p]-1) < DBUFF_READY)) p2 = 0;
			while ((p > p2) && (p > minPrecision)) {
				if (processedFrom[p-1] > processedFrom[p]) {
					processedFrom[p-1] = processedFrom[p];
				}
				p--;
			}
			if (p2 < minPrecision) break;
			processedFrom[p]--;
		}
		for (p = minPrecision; (p <= maxPrecision) && (processedTo[p] < dbuffer.begin); p++) {
			processedFrom[p] = dbuffer.begin;
		}
	}
	tmTaskLeave(&dmvTask);
}


// --- DATA PROCESSING ---


static inline unsigned char dlog2(unsigned int n) {
	if (!n) return 0;
	unsigned char l=-1;
	for (; n; n >>= 1) l++;
	return l;
}

static inline bool tryProcessLogFft(
		int pos,
		float *outputBuffer,
		unsigned char *precisionPtr) {
	double posSec = pos / msgOption_outputRate;
	unsigned char precision = rsContainsCnt(posSec, fftBlockLen/2);

	if (precision < *precisionPtr) {
		*precisionPtr = precision;
	}

	if (dbufferTransactionBegin(pos, precision)) {
		if (logFftProcess(posSec, precision, outputBuffer)) {
			*precisionPtr = precision;
			return true;
		} else {
			dbufferTransactionAbort(pos);
		}
	}
	return false;
}

static bool taskFunc() {

	int bufferPos = dsPlayerPosToColumn(playerPos);
	int visibleTo = bufferPos+msgOption_visibleAfter;
	if (visibleTo > dbuffer.end) visibleTo = dbuffer.end;
	int visibleFrom = bufferPos-msgOption_visibleBefore;
	if (visibleFrom < dbuffer.begin) visibleFrom = dbuffer.begin;
	if (visibleFrom < 0) visibleFrom = 0;

	bool logFftProcessed = false;
	float *buffer = malloc(dbuffer.columnLen * sizeof(float));

	int pos;
	unsigned char precision;

	if (!logFftProcessed) {
		precision = maxPrecision;
		for (pos = processedTo[maxPrecision]; pos < visibleTo; pos++) {
			if (tryProcessLogFft(pos, buffer, &precision)) {
				logFftProcessed = true;
				break;
			} else if (precision < minPrecision) {
				break;
			} else if (processedTo[precision] > pos) {
				pos = processedTo[precision]-1;
			}
		}
	}

	if (!logFftProcessed) {
		precision = maxPrecision;
		for (pos = processedFrom[maxPrecision]-1; (pos >= visibleFrom) && (pos >= 0); pos--) {
			if (tryProcessLogFft(pos, buffer, &precision)) {
				logFftProcessed = true;
				break;
			} else if (precision < minPrecision) {
				break;
			} else if (processedFrom[precision] < pos) {
				pos = processedFrom[precision]+1;
			}
		}
	}

	if (!logFftProcessed) {
		precision = maxPrecision;
		for (pos = (visibleTo>processedTo[maxPrecision] ? visibleTo : processedTo[maxPrecision]); pos < dbuffer.end; pos++) {
			if (tryProcessLogFft(pos, buffer, &precision)) {
				logFftProcessed = true;
				break;
			} else if (precision < minPrecision) {
				break;
			} else if (processedTo[precision] > pos) {
				pos = processedTo[precision]-1;
			}
		}
	}

	if (!logFftProcessed) {
		precision = maxPrecision;
		for (pos = (visibleFrom<processedFrom[maxPrecision] ? visibleFrom : processedFrom[maxPrecision])-1; (pos >= dbuffer.begin) && (pos >= 0); pos--) {
			if (tryProcessLogFft(pos, buffer, &precision)) {
				logFftProcessed = true;
				break;
			} else if (precision < minPrecision) {
				break;
			} else if (processedFrom[precision] < pos) {
				pos = processedFrom[precision]+1;
			}
		}
	}

	if (!logFftProcessed) {
		free(buffer);
		return false;
	}

	if (!dbufferTransactionCheck(pos)) {
		dbufferTransactionAbort(pos);
		free(buffer);
		return false;
	}

	createColumn(pos, buffer);
	free(buffer);

	bool ret = dbufferTransactionCommit(pos, precision);
	return ret;
}


/*
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
			double dblIndex=scalePosToInputIndex(y);
			double ratio=dblIndex;
			freq=scaleInputIndexToFreq(dblIndex);
			size_t index=(int)ratio;
			ratio-=index;
			value=buff[x*inputColumnLen+index]*(1-ratio) + buff[x*inputColumnLen+index+1]*ratio; // ?
			if (colorOvertones) {
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
*/
