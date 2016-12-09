// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <math.h>
#include "drawerScale.h"
#include "drawerBuffer.h"
#include "util.h"

double dsColumnToPlayerPosMultiplier;

double dsMinFreq, dsMaxFreq, dsA1Freq;
double dsOctaveOffset, dsSemitoneOffset, dsA1Index;

const bool isKeyWhite[] = {1,0,1,0,1, 1,0,1,0,1,0,1}; // 9 ~ A
struct dsOvertonesS dsOvertones[DS_OVERTONES_CNT];

static inline void setColumnOverlayColor(int index, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	if ((index >= 0) && (index < dbuffer.columnLen)) {
		dbufferColumnOverlay(index,0) = r;
		dbufferColumnOverlay(index,1) = g;
		dbufferColumnOverlay(index,2) = b;
		dbufferColumnOverlay(index,3) = a;
	}
}

extern void dsSetToneScale(double minFreq, double maxFreq, double a1Freq) {
	dsMinFreq = minFreq;
	dsMaxFreq = maxFreq;
	dsA1Freq  = a1Freq;
	dsOctaveOffset = dbuffer.columnLen / log2(dsMaxFreq/dsMinFreq);
	dsSemitoneOffset = dsOctaveOffset / 12;
	dsA1Index = 12*log2(dsA1Freq/dsMinFreq) * dsSemitoneOffset;

	double toneRadius = dsSemitoneOffset/2 - 2;
	if (toneRadius > 10) toneRadius = 10;
	if (toneRadius < 5) toneRadius = 0;
	int tone = -dsA1Index / dsSemitoneOffset -1;
	double index = dsA1Index + tone*dsSemitoneOffset;
	tone = ((tone % 12) + 12 + 9) % 12;
	for (; index-toneRadius < dbuffer.columnLen; index+=dsSemitoneOffset, tone = (tone+1)%12) {
		unsigned char color = (tone > 0 ? (isKeyWhite[tone] ? 128 : 64) : 255);
		for (int i = index-toneRadius+1; i < index+toneRadius; i++) {
			setColumnOverlayColor(i, color, color, color, 64);
		}
		setColumnOverlayColor(index-toneRadius, color, color, color, 128);
		setColumnOverlayColor(index+toneRadius, color, color, color, 128);
	}

	for (int i=0; i < DS_OVERTONES_CNT; i++) {
		double offset = log2(i+2) * dsOctaveOffset;
		dsOvertones[i].offset = offset;
		dsOvertones[i].offsetFract = offset - dsOvertones[i].offset;
	}
}

// --- SCALE CONVERSION ---
/*

static double scaleFreqToPosMultiplier;
static inline double scaleFreqToPos(double freq) {
	// scaleFreqToPosMultiplier=(double)height/log(scaleMaxFreq/scaleMinFreq);
	return log(freq/dsMinFreq)*scaleFreqToPosMultiplier;
}
static double scalePosToInputIndexMultiplier;
static double *scalePosToInputIndexCache=0;
#define scalePosToInputIndex(pos) scalePosToInputIndexCache[pos]
static inline double scalePosToInputIndexUncached(double pos) {
	// scalePosToInputIndexMultiplier=scaleMinFreq/(scaleMaxFreq-scaleMinFreq)*(inputColumnLenToDraw-1);
	return (pow(dsMaxFreq/dsMinFreq,pos/height)-1)*scalePosToInputIndexMultiplier;
}

static inline double scaleFreqToInputIndex(double freq) {
	return (freq-scaleMinFreq)/(dsMaxFreq-dsMinFreq)*(inputColumnLenToDraw-1);
}
static inline double scaleInputIndexToFreq(double index) {
	return index/(inputColumnLenToDraw-1)*(scaleMaxFreq-scaleMinFreq)+scaleMinFreq;
}


*/
// --- SCALE LABELS ---

/*
struct scaleLabels {
	size_t pos;
	bool main;
	char label[32];
} *scaleLabels=NULL;
size_t scaleLabelsCnt=0;


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
	scaleFreqToPosMultiplier=(double)height/log(scaleMaxFreq/scaleMinFreq);
	scalePosToInputIndexMultiplier=scaleMinFreq/(scaleMaxFreq-scaleMinFreq)*(inputColumnLenToDraw-1);

	free(scalePosToInputIndexCache);
	scalePosToInputIndexCache=utilMalloc(sizeof(double)*height);
	for (size_t i=0; i<height; i++)
		scalePosToInputIndexCache[i]=scalePosToInputIndexUncached(i);

	float multiplier, difference, freq;
	double freq1Pos;
	freq=scaleAnchoredFreq;
	if (scaleTones) {
		multiplier=pow(2,1.0/12);
		int tone=0, lastLabeledPos=-SCALE_LABELS_FONT_HEIGHT;
		for (; freq>=scaleMinFreq; freq/=multiplier, tone--);
		for (; freq<scaleMinFreq; freq*=multiplier, tone++);
		keyboardMinFreq=freq/pow(2,1.0/24);
		keyboardMinTone=((tone-4)%12+12)%12;
		free(scaleLabels);
		scaleLabelsCnt=(size_t)ceil(log(scaleMaxFreq/freq)/log(multiplier));
		scaleLabels=utilMalloc(sizeof(struct scaleLabels)*scaleLabelsCnt);
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
		multiplier=2;
		freq1Pos=scaleFreqToPos(1);
		for (; scaleFreqToPos(multiplier)-freq1Pos>=MIN_SCALE_LABELS_DIST; multiplier=sqrt(multiplier));
		for (; scaleFreqToPos(multiplier)-freq1Pos<MIN_SCALE_LABELS_DIST; multiplier=pow(multiplier,2));
		for (; freq>=scaleMinFreq; freq/=multiplier);
		for (; freq<scaleMinFreq; freq*=multiplier);
		free(scaleLabels);
		scaleLabelsCnt=(size_t)ceil(log(scaleMaxFreq/freq)/log(multiplier));
		scaleLabels=utilMalloc(sizeof(struct scaleLabels)*scaleLabelsCnt);
		for (size_t i=0; i<scaleLabelsCnt; freq*=multiplier, i++) {
			scaleLabels[i].pos=(size_t)(scaleFreqToPos(freq)+0.5);
			if (height-scaleLabels[i].pos>SCALE_LABELS_FONT_HEIGHT)
				sprintf(scaleLabels[i].label,"%0.2f Hz", freq);
			else
				scaleLabels[i].label[0]='\0';
			scaleLabels[i].main=true;
		}
	}
}

*/
