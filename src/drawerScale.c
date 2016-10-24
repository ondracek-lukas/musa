// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <math.h>
#include "drawerScale.h"

double dsColumnToPlayerPosMultiplier;

bool dsTones;
double dsMinFreq, dsMaxFreq, dsAnchoredFreq;

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
	scalePosToInputIndexCache=malloc(sizeof(double)*height);
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
	}
}

*/
