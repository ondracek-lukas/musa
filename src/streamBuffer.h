// MusA  Copyright (C) 2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#ifndef STREAM_BUFFER_H
#define STREAM_BUFFER_H

#include <stdio.h>
#include <stdbool.h>

#define STREAM_BUFFER_SIZE (2<<20)

struct streamBuffer {
	int begin; // position in the stream of the first sample in buffer
	int end;   // position in the stream of the first sample not in buffer
	int streamBegin, streamEnd;      // -1 if infinite
	float data[STREAM_BUFFER_SIZE];  // indexes are positions in stream mod STREAM_BUFFER_SIZE
};

#define sbPosToIndex(buffer,pos) (((pos)+STREAM_BUFFER_SIZE)%STREAM_BUFFER_SIZE)
#define sbValue(buffer,pos) (buffer)->data[sbPosToIndex(buffer,pos)]
#define sbWrapBetween(buffer,pos1,pos2) (((pos1)+STREAM_BUFFER_SIZE)/STREAM_BUFFER_SIZE != ((pos2)+STREAM_BUFFER_SIZE-1)/STREAM_BUFFER_SIZE)



//   [
//   *****
#define sbStreamContainsBegin(buffer,pos) (((buffer)->streamBegin < 0) || ((pos) >= (buffer)->streamBegin))
#define sbStreamContainsEnd(buffer,pos)     (((buffer)->streamEnd < 0) || ((pos) <= (buffer)->streamEnd))

//   [
// (((
#define sbAtStreamBegin(buffer) (((buffer)->streamBegin >= 0) && ((buffer)->begin <= (buffer)->streamBegin))
#define sbAtStreamEnd(buffer)     (((buffer)->streamEnd >= 0) && ((buffer)->end   >= (buffer)->streamEnd))

//   [ (      // streamBegin then begin
//     ***    // pos returning true
//   ( [
// *******
//   (
//   [
// *****
#define sbContainsBegin(buffer,pos) (((buffer)->begin <= (pos)) || sbAtStreamBegin(buffer))
#define sbContainsEnd(buffer,pos)   (((buffer)->end   >= (pos)) || sbAtStreamEnd(buffer))



extern bool sbRead(struct streamBuffer *sb, int begin, int end, float *outBuffer);


// Writing, to be used exclusively
extern void sbReset(struct streamBuffer *sb, int newPos, int newStreamBegin, int newStreamEnd);
extern void sbPreAppend(struct streamBuffer *sb, int newEnd);
extern void sbPostAppend(struct streamBuffer *sb, int newEnd);
extern void sbPrePrepend(struct streamBuffer *sb, int newBegin);
extern void sbPostPrepend(struct streamBuffer *sb, int newBegin);

extern void sbClear(struct streamBuffer *sb, int begin, int end);
extern void sbWrite(struct streamBuffer *sb, int begin, int end, float *data);

#endif
