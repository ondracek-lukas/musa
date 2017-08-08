// MusA  Copyright (C) 2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include "streamBuffer.h"
#include <string.h>

bool sbRead(struct streamBuffer *sb, int begin, int end, float *outBuffer) {
	if (!sbContainsBegin(sb, begin) || !sbContainsEnd(sb, end)) return false;
	if (!sbStreamContainsBegin(sb, begin)) {
		int size = sb->streamBegin - begin;
		memset(outBuffer, 0, size * sizeof(float));
		outBuffer += size;
		begin = sb->streamBegin;
	}
	if (!sbStreamContainsEnd(sb, end)) {
		memset(outBuffer + sb->streamEnd - begin, 0, end - sb->streamEnd);
		end = sb->streamEnd;
	}
	if (sbWrapBetween(sb, begin, end)) {
		int sizeToWrap = (sb->data + STREAM_BUFFER_SIZE - &sbValue(sb, begin));
		memcpy(outBuffer, &sbValue(sb, begin), sizeToWrap * sizeof(float));
		memcpy(outBuffer + sizeToWrap, sb->data, (end-begin-sizeToWrap) * sizeof(float));
	} else {
		memcpy(outBuffer, &sbValue(sb, begin), (end-begin)*sizeof(float));
	}
	if (!sbContainsBegin(sb, begin) || !sbContainsEnd(sb, end)) return false;
	return true;
}

void sbReset(struct streamBuffer *sb, int newPos, int newStreamBegin, int newStreamEnd) {
	sb -> begin = newPos;
	sb -> end   = newPos;
	sb -> streamBegin = newStreamBegin;
	sb -> streamEnd   = newStreamEnd;
}

void sbPreAppend(struct streamBuffer *sb, int newEnd) {
	if (newEnd - sb->begin > STREAM_BUFFER_SIZE) {
		sb->begin = newEnd - STREAM_BUFFER_SIZE;
		__sync_synchronize();
	}
}

void sbPostAppend(struct streamBuffer *sb, int newEnd) {
	__sync_synchronize();
	sb -> end = newEnd;
}

void sbPrePrepend(struct streamBuffer *sb, int newBegin) {
	if (sb->end - newBegin > STREAM_BUFFER_SIZE) {
		sb->end = newBegin + STREAM_BUFFER_SIZE;
		__sync_synchronize();
	}
}

void sbPostPrepend(struct streamBuffer *sb, int newBegin) {
	__sync_synchronize();
	sb -> begin = newBegin;
}

void sbClear(struct streamBuffer *sb, int begin, int end) {
	if (!sbStreamContainsBegin(sb, begin)) {
		int size = sb->streamBegin - begin;
		begin = sb->streamBegin;
	}
	if (!sbStreamContainsEnd(sb, end)) {
		end = sb->streamEnd;
	}
	if (sbWrapBetween(sb, begin, end)) {
		int sizeToWrap = (sb->data + STREAM_BUFFER_SIZE - &sbValue(sb, begin));
		memset(&sbValue(sb, begin), 0, sizeToWrap * sizeof(float));
		memset(sb->data, 0, (end-begin-sizeToWrap) * sizeof(float));
	} else {
		memset(&sbValue(sb, begin), 0, (end-begin)*sizeof(float));
	}
}

void sbWrite(struct streamBuffer *sb, int begin, int end, float *data) {
	if (!sbStreamContainsBegin(sb, begin)) {
		int size = sb->streamBegin - begin;
		data += size;
		begin = sb->streamBegin;
	}
	if (!sbStreamContainsEnd(sb, end)) {
		end = sb->streamEnd;
	}
	if (sbWrapBetween(sb, begin, end)) {
		int sizeToWrap = (sb->data + STREAM_BUFFER_SIZE - &sbValue(sb, begin));
		memcpy(&sbValue(sb, begin), data, sizeToWrap * sizeof(float));
		memcpy(sb->data, data + sizeToWrap, (end-begin-sizeToWrap) * sizeof(float));
	} else {
		memcpy(&sbValue(sb, begin), data, (end-begin)*sizeof(float));
	}
}
