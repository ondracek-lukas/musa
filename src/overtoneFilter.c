// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include "messages.h"
#include "drawerScale.h"
#include "mem.h"
#include <math.h>


void ofProcess(float *data) { // TODO: optimize
	static __thread float *harmOrig=NULL, *harmOrigB=NULL, *harmRelated=NULL, *overtoneCache=NULL;

	if (harmOrig == NULL) {
		harmOrig      = memMalloc(sizeof(float) * (DS_OVERTONES_CNT+2));
		harmOrigB     = memMalloc(sizeof(float) * (DS_OVERTONES_CNT+2));
		harmRelated   = memMalloc(sizeof(float) * (DS_OVERTONES_CNT+2));
		overtoneCache = memMalloc(sizeof(float) * DS_OVERTONES_CNT);
	}

	static float *dataB = NULL;
	{
		static float dataBLen = 0;
		if (dataBLen != msgOption_columnLen) {
			free(dataB);
			dataBLen = msgOption_columnLen;
			dataB = memMalloc(dataBLen * sizeof(float));
		}
	}

	for (int i = 0; i < msgOption_columnLen; i++) {
		dataB[i] = data[i];
	}
	for (int i = 0; i < msgOption_columnLen; i++) {
		for (int j = 0; j <= msgOption_overtoneBlur; j++) {
			if ((i+j < msgOption_columnLen) && (dataB[i+j]<data[i])) {
				dataB[i+j] = data[i];
			}
			if ((i-j >= 0) && (dataB[i-j] < data[i])) {
				dataB[i-j] = data[i];
			}
		}
	}


	for (int j=0; j < DS_OVERTONES_CNT; j++) {
		overtoneCache[j] = 0;
	}

	for (int i=-dsOvertones[DS_OVERTONES_CNT-1].offset-1; i < msgOption_columnLen; i++) {
		int j;
		
		// --- reading harmonics --

		if (i >= 0) {
			harmOrigB[0] = dataB[i];
			harmOrig[0]  = data[i];
		} else {
			harmOrig[0]  = 0;
			harmOrigB[0] = 0;
		}
		for (j=1; j <= DS_OVERTONES_CNT; j++) {
			float value  = 0;
			float valueB = 0;
			int k = i + dsOvertones[j-1].offset;
			float ratio = dsOvertones[j-1].offsetFract;
			if ((k >= 0) && (k < msgOption_columnLen)) {
				value  += data[k]  * (1-ratio);
				valueB += dataB[k] * (1-ratio);
			}
			k++;
			if ((k >= 0) && (k < msgOption_columnLen)) {
				value  += data[k]  * ratio;
				valueB += dataB[k] * ratio;
			}
			harmOrig[j]  = value;
			harmOrigB[j] = valueB;
		}
		harmOrigB[DS_OVERTONES_CNT+1] = 0;
		harmOrig[DS_OVERTONES_CNT+1]  = 0;


		// --- filtering ---

		float minAvg;
		float minAvgRest;
		int minAvgIndex;
		{
			float sum = 0;
			int   cnt = 0;
			for (int k = 1; k <= DS_OVERTONES_CNT; k ++) {
				sum += harmOrigB[k];
			}
			minAvg = sum/DS_OVERTONES_CNT;
			float totalSum = sum;
			float totalAvg = minAvg;
			minAvgIndex = DS_OVERTONES_CNT+1;
			minAvgRest  = 0;
			for (j=1; j<=DS_OVERTONES_CNT; j++) {
				sum=0; cnt=0;
				for (int k = j; k <= DS_OVERTONES_CNT; k += j+1) {
					sum += harmOrigB[k];
					cnt ++;
				}
				float avg = (totalSum-sum)/(DS_OVERTONES_CNT-cnt);
				if (minAvg > avg) {
					minAvg = avg;
					minAvgRest = sum/cnt;
					minAvgIndex = j;
				}
			}
			if (minAvg * msgOption_overtoneThreshold/100 > totalAvg) {
				minAvg = totalAvg;
				minAvgIndex = DS_OVERTONES_CNT+1;
				minAvgRest = 0;
			}
		}

		float opacity = 1;
		if (harmOrigB[0] * msgOption_overtoneRatio/100 < minAvg) {
			opacity = harmOrigB[0] * msgOption_overtoneRatio/100 / minAvg;
		}
		for (j=1; j<=DS_OVERTONES_CNT; j++) {
			if (j % (minAvgIndex+1) == minAvgIndex) {
				harmRelated[j] = harmOrig[j] * minAvg / minAvgRest * opacity;
			} else {
				harmRelated[j] = harmOrig[j] * opacity;
			}
		}


		// --- writing harmonics ---

		float sum = 0;
		float relatedSum = 0;
		for (j=1; j<=DS_OVERTONES_CNT; j++) {
			sum         += harmOrig[j];
			relatedSum  += harmRelated[j];

			float value = harmOrig[j] - harmRelated[j];
			int k = i + dsOvertones[j-1].offset;
			float ratio = dsOvertones[j-1].offsetFract;
			if ((k >= 0) && (k < msgOption_columnLen)) {
				data[k] = overtoneCache[j-1] * ratio + value * (1-ratio);
				overtoneCache[j-1] = value;
			}
		}
		if (i >= 0) {
			data[i] += relatedSum * msgOption_overtoneAddition/100;
		}
	}

}
