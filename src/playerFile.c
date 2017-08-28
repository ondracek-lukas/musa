// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <pthread.h>

#include "player.h"
#include "playerFile.h"
#include "playerDevice.h"

#include "messages.h"
#include "taskManager.h"
#include "consoleOut.h"
#include "util.h"

#define MIN_AFTER_POS  (PLAYER_BUFFER_SIZE/2)
#define MAX_AFTER_POS  (PLAYER_BUFFER_SIZE*3/4)
#define MIN_BEFORE_POS (PLAYER_BUFFER_SIZE/8)
#define MAX_BEFORE_POS (PLAYER_BUFFER_SIZE/4)


struct taskInfo playerFileTask = TM_TASK_INITIALIZER(false, true);
struct taskInfo playerFileThreadTask = TM_TASK_INITIALIZER(false, true);
static pthread_t thread;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static bool stopRequested = false;

AVFormatContext * formatContext = NULL;
AVCodecContext *codecContext = NULL;
SwrContext *swrContext = NULL;
int streamIndex = -1;

bool threadWaiting = false;

static void *avReader(void *none);
static bool taskFunc();

static __attribute__((constructor)) void init() {
	av_register_all();
	tmTaskRegister(&playerFileTask, taskFunc, 0);
	pthread_create(&thread, NULL, avReader, NULL);
}

static __attribute__((destructor)) void fin() {
	pthread_mutex_lock(&mutex);
	stopRequested = true;
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mutex);
	pthread_join(thread, NULL);
}

#define ERR(...) {consolePrintErrMsg(__VA_ARGS__); playerFileClose(); return false; }
static bool fileOpen();

bool playerFileOpen(char *filename) {
	if (avformat_open_input(&formatContext, filename, NULL, NULL) != 0)
		ERR("Cannot open file %s.", filename);
	return fileOpen();
}
struct myIOContext {
	const char *data;
	size_t size;
	size_t pos;
};
static int myIORead(void *data, uint8_t *buf, int buf_size) {
	struct myIOContext *context = data;
	if (context->pos + buf_size > context->size) {
		buf_size = context->size - context->pos;
	}
	memcpy(buf, context->data + context->pos, buf_size);
	context->pos += buf_size;
	return buf_size;
}
static int64_t myIOSeek(void *data, int64_t pos, int whence) {
	struct myIOContext *context = data;
	size_t newPos;
	switch (whence) {
		case SEEK_SET:
			newPos = pos;
			break;
		case SEEK_CUR:
			newPos = context->pos + pos;
			break;
		case SEEK_END:
			newPos = context->size + pos;
			break;
		case AVSEEK_SIZE:
			return context->size;
		default:
			return -1;
	}
	if ((newPos >= 0) && (newPos <= context->size)) {
		context->pos = newPos;
		return context->pos;
	} else {
		return -1;
	}
}
bool playerDataOpen(char *filename, const char *data, size_t size) {
	struct myIOContext *context = utilMalloc(sizeof(struct myIOContext));
	context->data = data;
	context->size = size;
	context->pos  = 0;

	size_t bufferSize = 4096;
	uint8_t *buffer = av_malloc(bufferSize);
	AVIOContext *ioContext = avio_alloc_context(
			buffer, bufferSize,
			0, context, myIORead, 0, myIOSeek);
	formatContext = avformat_alloc_context();
	formatContext->pb = ioContext;
	formatContext->flags |= AVFMT_FLAG_CUSTOM_IO;

	if (avformat_open_input(&formatContext, filename, NULL, NULL) != 0)
		ERR("Cannot open file %s.", filename);
	return fileOpen();
}

static bool fileOpen() { // needs formatContext to be open

	if (avformat_find_stream_info(formatContext, NULL) < 0) {
		ERR("Cannot find stream information.");
	}
	streamIndex = -1;
	for (int i=0; i < formatContext->nb_streams; i++) {
		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			if (streamIndex < 0) {
				streamIndex = i;
			} else {
				msgSend_printErr("Warning: multiple audio streams found.");
			}
		}
	}
	if (streamIndex < 0) {
		ERR("No audio stream found.");
	}

	AVCodec *codec = avcodec_find_decoder(formatContext->streams[streamIndex]->codecpar->codec_id);
	if (!codec) {
		ERR("Unsupported codec.");
	}

	codecContext = avcodec_alloc_context3(codec);
	if (!codecContext) {
		ERR("Cannot allocate codec context.");
	}
	if (avcodec_parameters_to_context(codecContext, formatContext->streams[streamIndex]->codecpar) != 0) {
		ERR("Cannot initialize codec.");
	}


	playerSampleRate = codecContext->sample_rate;
	playerDuration = ((double)
		formatContext->streams[streamIndex]->time_base.num) *
		formatContext->streams[streamIndex]->duration /
		formatContext->streams[streamIndex]->time_base.den;

	
	if (avcodec_open2(codecContext, codec, NULL) != 0) {
		ERR("Cannot initialize decoder.");
	}

	swrContext = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_FLT, playerSampleRate,
			av_get_default_channel_layout(codecContext->channels),
			codecContext->sample_fmt,
			codecContext->sample_rate,
			0, NULL);
	if (!swrContext) {
		ERR("Cannot allocate swrContext.");
	}
	if (swr_init(swrContext)) {
		ERR("Cannot initialize swr.");
	}

	sbReset(&playerBuffer, 0, 0, -1);

	playerPos=0;

	playerFileTask.active = true;
	playerFileThreadTask.active = true;
	return true;
}
#undef ERR


static void fileClose() {
	playerFileTask.active = false;
	playerFileThreadTask.active = false;
	if (codecContext) {
		avcodec_close(codecContext);
		avcodec_free_context(&codecContext);
	}
	if (swrContext) {
		swr_free(&swrContext);
	}
}
void playerFileClose() {
	fileClose();
	if (formatContext) {
		avformat_close_input(&formatContext);
	}
}
void playerDataClose() {
	fileClose();
	if (formatContext) {
		free(formatContext->pb->opaque);
		av_free(formatContext->pb->buffer);
		av_free(formatContext->pb);
		avformat_close_input(&formatContext);
	}
}


static bool taskFunc() {
	if (threadWaiting &&
			(!sbContainsBegin(&playerBuffer, playerPos - MIN_BEFORE_POS) ||
			 !sbContainsEnd(&playerBuffer, playerPos + MIN_AFTER_POS))) {
		pthread_cond_broadcast(&cond);
		return true;
	}
	return false;
}


#define ERR(...) {consolePrintErrMsg(__VA_ARGS__); playerFileTask.active = false; playerFileThreadTask.active=false; msgSend_close(); tmTaskLeave(&playerFileThreadTask); continue; }
static void *avReader(void *none) {
	AVFrame* frame = av_frame_alloc();
	if (!frame) utilExitErr("Cannot allocate frame.");
	AVPacket packet;
	av_init_packet(&packet);
	bool done = false;
	int afterSeek = 0;
	int err;

	while (true) {
		if (stopRequested || done || !tmTaskEnter(&playerFileThreadTask)) {
			pthread_mutex_lock(&mutex);
			if (stopRequested) {
				pthread_mutex_unlock(&mutex);
				break;
			}
			if (done || !tmTaskEnter(&playerFileThreadTask)) {
				done = false;
				threadWaiting = true;
				pthread_cond_wait(&cond, &mutex);
				threadWaiting = false;
				pthread_mutex_unlock(&mutex);
				continue;
			} else {
				pthread_mutex_unlock(&mutex);
			}
		}

		err = avcodec_receive_frame(codecContext, frame);
		switch (err) {
			case 0:
				{
					if (afterSeek == 2) {
						int pos = ((long long int)frame->pts) * playerSampleRate *
							formatContext->streams[streamIndex]->time_base.num /
							formatContext->streams[streamIndex]->time_base.den;
						sbReset(&playerBuffer, pos, playerBuffer.streamBegin, playerBuffer.streamEnd);
						afterSeek = 0;
					}
					int cnt = swr_get_out_samples(swrContext, frame->nb_samples);
					sbPreAppend(&playerBuffer, playerBuffer.end + cnt);
					uint8_t *buffer = (uint8_t *)&sbValue(&playerBuffer, playerBuffer.end);
					int maxCnt = PLAYER_BUFFER_SIZE - sbPosToIndex(&playerBuffer, playerBuffer.end);
					cnt = swr_convert(swrContext, &buffer, maxCnt, (const uint8_t **)&frame->data, frame->nb_samples);
					if (cnt < 0) {
						av_frame_unref(frame);
						ERR("Cannot convert stream to mono.");
					}
					sbPostAppend(&playerBuffer, playerBuffer.end + cnt);
					tmResume();
					while (cnt > 0) {
						uint8_t *buffer = (uint8_t *) &sbValue(&playerBuffer, playerBuffer.end);
						int maxCnt = PLAYER_BUFFER_SIZE - sbPosToIndex(&playerBuffer, playerBuffer.end);
						cnt = swr_convert(swrContext, &buffer, maxCnt, NULL, 0);
						if (cnt < 0) {
							av_frame_unref(frame);
							ERR("Cannot convert stream to mono.");
						}
						sbPostAppend(&playerBuffer, playerBuffer.end + cnt);
					}
					av_frame_unref(frame);
					if (playerPlaying) {
						playerDeviceStart();
					}
					tmTaskLeave(&playerFileThreadTask);
					continue;
				}
			case AVERROR(EAGAIN):
				break;
			case AVERROR_EOF:
				avcodec_flush_buffers(codecContext);
				if (afterSeek != 1) {
					playerBuffer.streamEnd = playerBuffer.end;
					done=true;
					tmTaskLeave(&playerFileThreadTask);
					continue;
				}
				break;
			default:
				ERR("Cannot receive data from codec.")
		}

		if (!afterSeek) {
			if ((!sbContainsBegin(&playerBuffer, playerPos-MIN_BEFORE_POS)) ||
					(!sbContainsEnd(&playerBuffer, playerPos-MAX_BEFORE_POS))) {
				double seekPts = ((double)playerPos-MAX_BEFORE_POS)/playerSampleRate *
						formatContext->streams[streamIndex]->time_base.den /
						formatContext->streams[streamIndex]->time_base.num;
				if (seekPts < 0) seekPts = 0;
				err = av_seek_frame(formatContext, streamIndex, seekPts, AVSEEK_FLAG_BACKWARD);
				if (err < 0) {
					ERR("Cannot seek.");
				}
				if (avcodec_send_packet(codecContext, NULL) != 0) {
					ERR("Cannot send data to codec after seek.");
				}
				afterSeek = 1;
				tmTaskLeave(&playerFileThreadTask);
				continue;
			} else if (sbContainsEnd(&playerBuffer, playerPos+MAX_AFTER_POS)) {
				done = true;
				tmTaskLeave(&playerFileThreadTask);
				continue;
			}
		}

		while (true) {
			err = av_read_frame(formatContext, &packet);
			if (err) break;
			if (packet.stream_index != streamIndex) {
				av_packet_unref(&packet);
			} else {
				break;
			}
		}
		switch (err) {
			case 0:
				err = avcodec_send_packet(codecContext, &packet);
				if (afterSeek == 1) {
					afterSeek = 2;
				}
				break;
			case AVERROR_EOF:
				err = avcodec_send_packet(codecContext, NULL);
				break;
			default:
				ERR("Cannot read input data.");
		}

		if (err != 0) {
			ERR("Cannot send data to codec.");
		}

		tmTaskLeave(&playerFileThreadTask);
	}

	return NULL;
}
#undef ERR
