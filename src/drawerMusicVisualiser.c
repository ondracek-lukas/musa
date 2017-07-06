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

#define MAX_PRECISION_LEVELS 32

static int visibleBefore, visibleAfter,
	 processedFrom[MAX_PRECISION_LEVELS],
	 processedTo[MAX_PRECISION_LEVELS];
static unsigned char minPrecision, maxPrecision;


struct taskInfo dmvTask = TM_TASK_INITIALIZER(false, false);


// --- COLORING ---

static unsigned char colorScale[1024*3]={255}; // will be initialized below

static inline double toLogScale(double value) {
	// static float max = 0.001; // XXX
	// if (value > max) max = value; value /= max;
	value = log2(value)/12 + 1;
	if (value > 1) value = 1;
	if (!(value >= 0)) value = 0;
	return value;
}

static inline void toColorScale(float value, unsigned char *color) {
	unsigned valueI = value*1024;
	if (valueI>=1024)
		valueI=1023;
	valueI*=3;

	color[0] = colorScale[valueI];
	color[1] = colorScale[valueI+1];
	color[2] = colorScale[valueI+2];
}

void dmvCreatePreview(int column) {
	int playerFrom = dsColumnToPlayerPos(column-1);
	int playerTo = dsColumnToPlayerPos(column+1);
	if ((playerBuffer.begin>playerFrom) || (playerBuffer.end<=playerTo)) {
		// printf("%d > %d or %d <= %d\n", playerBuffer.begin, playerFrom, playerBuffer.end, playerTo);
		return;
	}
	double sum=0;
	for (int i = playerFrom; i<playerTo; i++) {
		double a=playerBuffer(i);
		sum += a*a;
	}
	sum /= (playerTo-playerFrom);
	if ((playerBuffer.begin>playerFrom) || (playerBuffer.end<playerTo)) {
		return;
	}

	toColorScale(toLogScale(sum), &dbufferPreview(column,0));
	dbufferPreviewCreated(column)=true;
}


static void createColumn(int column, float *logFftResult) {
	float overtoneSub[DS_OVERTONES_CNT];
	for (int i=0; i<DS_OVERTONES_CNT; i++) {
		overtoneSub[i] = 0;
	}
	for (int i=0; i<dbuffer.columnLen; i++) {
		double amp = logFftResult[i];
		double addedAmp = 0;
		/* // XXX Overtone filtering for one tone at a time
		double evenAmp = 0;
		double oddAmp = 0;
		for (int j=0; j<DS_OVERTONES_CNT; j++) {
			int offset = dsOvertones[j].offset;
			float fract = dsOvertones[j].offsetFract;
			if (i + offset + 1 >= dbuffer.columnLen) break;
			float overtoneAmp = logFftResult[i + offset] * (1-fract);
			overtoneAmp      += logFftResult[i + offset +1] * fract;

			if (overtoneAmp > amp) overtoneAmp = amp;

			logFftResult[i + offset] -= overtoneSub[j] + overtoneAmp * (1-fract);
			if (logFftResult[i+offset] <= 0) {
				logFftResult[i+offset] = 0;
			}
			overtoneSub[j] = overtoneAmp * fract;

			double ampToAdd = overtoneAmp/(2<<j);
			addedAmp += ampToAdd;
			if (i%2) {
				oddAmp += ampToAdd;
			} else {
				evenAmp += ampToAdd;
			}
		}
		oddAmp  = toLogScale(oddAmp*3/2);
		evenAmp = toLogScale(evenAmp*3);
		/*/
		addedAmp = amp;
		//*/
		amp     = toLogScale((amp + addedAmp)/2);

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

//void dmvResize(int columnLen, int visibleBefore, int visibleAfter, double minFreq, double maxFreq, double a1Freq) {
void dmvReset() { // pause dmvTask and drawerMainTask before
	if ((msgOption_columnLen<=0) || (msgOption_minFreq > msgOption_maxFreq)) { // XXX check elsewhere
		dmvTask.active = false;
		return;
	}
	dbufferRealloc(msgOption_columnLen);
	visibleBefore = msgOption_visibleBefore;
	visibleAfter = msgOption_visibleAfter;
	int pos = dsPlayerPosToColumn(playerPos);
	for (int i=0; i<MAX_PRECISION_LEVELS; i++) {
		processedTo[i] = processedFrom[i] = pos;
	}

	dsSetToneScale(msgOption_minFreq, msgOption_maxFreq, msgOption_a1Freq);
	logFftReset(
			msgOption_minFreq  / 44100.0,
			msgOption_maxFreq  / 44100.0,
			msgOption_columnLen, &minPrecision, &maxPrecision);
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


static inline int tryLoadInput(
		int pos,
		float *inputBuffer,
		unsigned char *precisionOut) {
	if (pos<0) return 0b01; // no previous will be successful
	int posPlayer = dsColumnToPlayerPos(pos);
	int posPlayerFrom = playerBuffer.begin;
	int posPlayerTo = playerBuffer.end;
	unsigned char precision;
	if (posPlayer - posPlayerFrom < posPlayerTo - posPlayer) {
		if (posPlayer < posPlayerFrom) {
			precision = 0;
		} else {
			precision = dlog2(posPlayer - posPlayerFrom);
		}
		if (precision < minPrecision) return 0b01; // no previous will be successfull
	} else {
		if (posPlayerTo < posPlayer) {
			precision = 0;
		} else {
			precision = dlog2(posPlayerTo - posPlayer);
		}
		if (precision < minPrecision) return 0b10; // no further will be successfull
	}
	if (precision > maxPrecision) {
		precision = maxPrecision;
	}
	posPlayerFrom = posPlayer - (1 << precision); // inclusive
	posPlayerTo = posPlayer + (1 << precision);   // exclusive
	if (dbufferTransactionBegin(pos, precision)) {
		if (playerBufferWrapBetween(posPlayerFrom, posPlayerTo)) {
			int sizeToWrap = (playerBuffer.data + PLAYER_BUFFER_SIZE - &playerBuffer(posPlayerFrom));
			memcpy(inputBuffer, &playerBuffer(posPlayerFrom), sizeToWrap * sizeof(float));
			memcpy(inputBuffer + sizeToWrap, playerBuffer.data, (posPlayerTo-posPlayerFrom-sizeToWrap) * sizeof(float));
			//loadInput(precision, &playerBuffer(posPlayerFrom), playerBuffer.data + PLAYER_BUFFER_SIZE);
			//loadInput(precision, playerBuffer.data, &playerBuffer(posPlayerTo));
		} else {
			memcpy(inputBuffer, &playerBuffer(posPlayerFrom), (posPlayerTo-posPlayerFrom)*sizeof(float));
			//loadInput(precision, &playerBuffer(posPlayerFrom), &playerBuffer(posPlayerTo));
		}
		if ((playerBuffer.end < posPlayerTo) || (playerBuffer.begin > posPlayerFrom)) {
			dbufferTransactionAbort(pos);
			//loadInput(0, NULL, NULL);
			return 0b11;  // try other
		}
		*precisionOut = precision;
		return 0b00; // success
	}
	return 0b11; // try other
}

static bool taskFunc() {

	int bufferPos = dsPlayerPosToColumn(playerPos);
	int visibleTo = bufferPos+visibleAfter;
	if (visibleTo > dbuffer.end) visibleTo = dbuffer.end;
	int visibleFrom = bufferPos-visibleBefore;
	if (visibleFrom < dbuffer.begin) visibleFrom = dbuffer.begin;
	if (visibleFrom < 0) visibleFrom = 0;

	bool inputLoaded = false;
	float *inputBuffer = malloc(sizeof(float)*2u<<maxPrecision);
	int pos;
	unsigned char precision;

	if (!inputLoaded) {
		for (pos = processedTo[maxPrecision]; pos < visibleTo; pos++) {
			unsigned char p;
			switch (tryLoadInput(pos, inputBuffer, &precision)) {
				case 0b00: // success
					inputLoaded = true;
					break;
				case 0b01: // no previous will success
				case 0b11: // try other
					p = dbufferPrecision(pos);
					if (p > maxPrecision) p = maxPrecision;
					if ((p >= minPrecision) && (processedTo[p] > pos)) pos = processedTo[p]-1;
					continue;
				case 0b10: // no further will success
					pos = INT_MAX-1;
					break;
			}
			if (inputLoaded) break;
		}
	}

	if (!inputLoaded) {
		for (pos = processedFrom[maxPrecision]-1; pos >= visibleFrom; pos--) {
			unsigned char p;
			switch (tryLoadInput(pos, inputBuffer, &precision)) {
				case 0b00: // success
					inputLoaded = true;
					break;
				case 0b10: // no further will success
				case 0b11: // try other
					p = dbufferPrecision(pos);
					if (p > maxPrecision) p = maxPrecision;
					if ((p >= minPrecision) && (processedFrom[p] < pos)) pos = processedFrom[p]+1;
					continue;
				case 0b01: // no previous will success
					pos = INT_MIN+1;
					break;
			}
			if (inputLoaded) break;
		}
	}

	if (!inputLoaded) {
		for (pos = (visibleTo>processedTo[maxPrecision] ? visibleTo : processedTo[maxPrecision]); pos < dbuffer.end; pos++) {
			unsigned char p;
			switch (tryLoadInput(pos, inputBuffer, &precision)) {
				case 0b00: // success
					inputLoaded = true;
					break;
				case 0b01: // no previous will success
				case 0b11: // try other
					p = dbufferPrecision(pos);
					if (p > maxPrecision) p = maxPrecision;
					if ((p >= minPrecision) && (processedTo[p] > pos)) pos = processedTo[p]-1;
					continue;
				case 0b10: // no further will success
					pos = INT_MAX-1;
					break;
			}
			if (inputLoaded) break;
		}
	}

	if (!inputLoaded) {
		for (pos = (visibleFrom<processedFrom[maxPrecision] ? visibleFrom : processedFrom[maxPrecision])-1; pos >= dbuffer.begin; pos--) {
			unsigned char p;
			switch (tryLoadInput(pos, inputBuffer, &precision)) {
				case 0b00: // success
					inputLoaded = true;
					break;
				case 0b10: // no further will success
				case 0b11: // try other
					p = dbufferPrecision(pos);
					if (p > maxPrecision) p = maxPrecision;
					if ((p >= minPrecision) && (processedFrom[p] < pos)) pos = processedFrom[p]+1;
					continue;
				case 0b01: // no previous will success
					pos = INT_MIN+1;
					break;
			}
			if (inputLoaded) break;
		}
	}

	if (!inputLoaded) {
		free(inputBuffer);
		return false;
	}

	float *outputBuffer = malloc(dbuffer.columnLen * sizeof(float));

	logFftProcess(precision, inputBuffer, outputBuffer);

	if (dbufferTransactionCheck(pos)) {
		createColumn(pos, outputBuffer);
		dbufferTransactionCommit(pos, precision);
	} else {
		dbufferTransactionAbort(pos);
	}

	free(inputBuffer);
	free(outputBuffer);

	return true;
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
