// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <math.h>
#include <float.h>
#include <limits.h>
#include "logFft.h"
#include "drawerScale.h"
#include "drawerBuffer.h"
#include "player.h"


static int visibleBefore, visibleAfter, processedFrom, processedTo;




// --- COLORING ---

static unsigned char colorScale[1024*3]={255}; // will be initialized below

void dmvCreatePreview(int column) {
	int playerFrom = dsColumnToPlayerPos(column-1);
	int playerTo = dsColumnToPlayerPos(column+1);
	if ((playerBuffer.begin>playerFrom) || (playerBuffer.end<=playerTo)) {
		// printf("%d > %d or %d <= %d\n", playerBuffer.begin, playerFrom, playerBuffer.end, playerTo);
		return;
	}
	double sum=0, vol=FLT_MIN;
	for (int i = playerFrom; i<playerTo; i++) {
		double a=playerBuffer(i);
		sum += a*a;
		if (vol < a) {
			vol=a;
		}
	}
	sum /= (playerTo-playerFrom)*vol;
	//sum = log(sum);
	if ((playerBuffer.begin>playerFrom) || (playerBuffer.end<playerTo)) {
		return;
	}
	unsigned valueI = sum*1024;
	if (valueI>=1024)
		valueI=1023;
	valueI*=3;
	dbufferPreview(column,0) = colorScale[valueI];
	dbufferPreview(column,1) = colorScale[valueI+1];
	dbufferPreview(column,2) = colorScale[valueI+2];
	dbufferPreviewCreated(column)=true;
}


static void createColumn(int column, float *logFftResult) {
	for (int i=0; i<dbuffer.columnLen; i++) {
		unsigned valueI = logFftResult[i]*1024;
		if (valueI>=1024)
			valueI=1023;
		valueI*=3;

		dbuffer(column, i, 0) = colorScale[valueI];
		dbuffer(column, i, 1) = colorScale[valueI+1];
		dbuffer(column, i, 2) = colorScale[valueI+2];
	}
}



// --- INIT FUNCTIONS ---

void dmvInit() {
	logFftInit();
	processedFrom = 0;
	processedTo = 0;

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


static int newColumnLen, newVisibleBefore, newVisibleAfter;
static void onRestart();
static logFftDataExchangeCallbackT onDataExchange;
void dmvResize(int columnLen, int visibleBefore, int visibleAfter) {
	dbuffer.dataInvalid = true;
	newColumnLen = columnLen;
	newVisibleBefore = visibleBefore;
	newVisibleAfter  = visibleAfter;
	logFftStop();
}
void dmvRefresh() {
	if (logFftIsRunning()) {
		if ((processedTo < dbuffer.begin) || (processedFrom > dbuffer.end)) {
			processedTo = processedFrom = dsPlayerPosToColumn(playerPos);
		} else {
			if (processedTo > dbuffer.end) {
				processedTo = dbuffer.end;
			} else {
				while ((processedTo < dbuffer.end) && (dbufferState(processedTo) >= DBUFF_PROCESSED)) {
					processedTo++;
				}
			}
			if (processedFrom < dbuffer.begin) {
				processedFrom = dbuffer.begin;
			} else {
				while ((processedFrom > dbuffer.begin) && (dbufferState(processedFrom-1) >= DBUFF_PROCESSED)) {
					processedFrom--;
				}
			}
		}

		logFftResume();
	} else {
		dbufferRealloc(newColumnLen);
		visibleBefore = newVisibleBefore;
		visibleAfter = newVisibleAfter;
		processedTo = processedFrom = dsPlayerPosToColumn(playerPos);
		logFftStart(0.0025, 0.05, newColumnLen, onDataExchange);
	}
}


// --- DATA PROCESSING ---

struct column {
	int pos;
};

static inline int tryLoadInput(
		int pos,
		size_t neighbourhoodNeeded,
		void (*loadInput)(float *data, float *endData)) {
	if (pos<0) return 0b01; // no previous will be successful
	if (dbufferTransactionBegin(pos)) {
		int posPlayer = dsColumnToPlayerPos(pos);
		int posPlayerFrom = posPlayer - neighbourhoodNeeded; // inclusive
		int posPlayerTo   = posPlayer + neighbourhoodNeeded; // exclusive
		if (playerBuffer.end < posPlayerTo) {
			dbufferTransactionAbort(pos);
			return 0b10;  // no further will be successful
		}
		if (playerBuffer.begin > posPlayerFrom) {
			//pos = dsPlayerPosToColumn(playerBuffer.begin)+neighbourhoodNeeded - 1;
			dbufferTransactionAbort(pos);
			return 0b01;  // no previous will be successful
		}
		if (playerBufferWrapBetween(posPlayerFrom, posPlayerTo)) {
			loadInput(&playerBuffer(posPlayerFrom), playerBuffer.data + PLAYER_BUFFER_SIZE);
			loadInput(playerBuffer.data, &playerBuffer(posPlayerTo));
		} else {
			loadInput(&playerBuffer(posPlayerFrom), &playerBuffer(posPlayerTo));
		}
		if ((playerBuffer.end < posPlayerTo) || (playerBuffer.begin > posPlayerFrom)) {
			dbufferTransactionAbort(pos);
			loadInput(NULL, NULL);
			return 0b11;  // try other
		}
		return 0b00; // success
	}
	return 0b11; // try other
}

static bool onDataExchange(
		void **info,
		float *resultData,
		size_t neighbourhoodNeeded,
		void (*loadInput)(float *data, float *dataEnd)) {
	struct column *column = *info;
	int pos;
	if (!column) {
		column = malloc(sizeof(struct column));
		column->pos = -1;
		*info = column;
		pos=column->pos;
	} else if (column->pos >= 0) {
		pos=column->pos;
		if (resultData) {
			if (dbufferTransactionCheck(pos)) {
				createColumn(pos, resultData);
				dbufferTransactionCommit(pos);
			} else {
				dbufferTransactionAbort(pos);
			}
		} else {
			dbufferTransactionAbort(pos);
		}
	}


	int bufferPos = dsPlayerPosToColumn(playerPos);
	int visibleTo = bufferPos+visibleAfter;
	if (visibleTo > dbuffer.end) visibleTo = dbuffer.end;
	int visibleFrom = bufferPos-visibleBefore;
	if (visibleFrom < dbuffer.begin) visibleFrom = dbuffer.begin;
	if (visibleFrom < 0) visibleFrom = 0;

	int tmp=0;

	for (pos = processedTo; pos < visibleTo; pos++) {
		tmp++;
		switch (tryLoadInput(pos, neighbourhoodNeeded, loadInput)) {
			case 0b00: // success
				column->pos = pos;
				return true;
			case 0b11: // try other
			case 0b01: // no previous will success
				continue;
			case 0b10: // no further will success
				pos = INT_MAX-1;
				break;
		}
	}

	for (pos = processedFrom-1; pos >= visibleFrom; pos--) {
		tmp++;
		switch (tryLoadInput(pos, neighbourhoodNeeded, loadInput)) {
			case 0b00: // success
				column->pos = pos;
				return true;
			case 0b11: // try other
			case 0b10: // no further will success
				continue;
			case 0b01: // no previous will success
				pos = INT_MIN+1;
				break;
		}
	}

	for (pos = (visibleTo>processedTo ? visibleTo : processedTo); pos < dbuffer.end; pos++) {
		tmp++;
		switch (tryLoadInput(pos, neighbourhoodNeeded, loadInput)) {
			case 0b00: // success
				column->pos = pos;
				return true;
			case 0b11: // try other
			case 0b01: // no previous will success
				continue;
			case 0b10: // no further will success
				pos = INT_MAX-1;
				break;
		}
	}

	for (pos = (visibleFrom<processedFrom ? visibleFrom : processedFrom)-1; pos >= dbuffer.begin; pos--) {
		tmp++;
		switch (tryLoadInput(pos, neighbourhoodNeeded, loadInput)) {
			case 0b00: // success
				column->pos = pos;
				return true;
			case 0b11: // try other
			case 0b10: // no further will success
				continue;
			case 0b01: // no previous will success
				pos = INT_MIN+1;
				break;
		}
	}

	column->pos = -1;
	return false;
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
