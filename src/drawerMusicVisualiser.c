// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <float.h>
#include "drawerScale.h"
#include "drawerBuffer.h"
#include "player.h"

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
	unsigned char value = sum*255;
	if ((playerBuffer.begin>playerFrom) || (playerBuffer.end<playerTo)) {
		return;
	}
	dbufferPreview(column,0)=value;
	dbufferPreview(column,1)=value;
	dbufferPreview(column,2)=value;
	dbufferPreviewCreated(column)=true;
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
