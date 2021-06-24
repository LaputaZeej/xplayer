//
// Created by xpl on 2021/6/24.
//

#ifndef LAPUTA_FFMPEG_AUDIOCHANNEL_H
#define LAPUTA_FFMPEG_AUDIOCHANNEL_H


#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

class AudioChannel : public BaseChannel {
    friend void *audio_Decode_t(void *args);

    friend void *audio_Play_t(void *args);

    friend void bqPlayerCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext);

public:
    AudioChannel(int channelId, JavaCallHelper *helper, AVCodecContext *avCodecContext,
                 const AVRational &base);

    virtual ~AudioChannel();

public:
    virtual void play();

    virtual void stop();

    virtual void decode();

private:
    void realDecode();

    void realPlay();

    int getData();

private:
    pthread_t audioDecodeTask, audioPlayTask;
    SwrContext* swrContext;
    uint8_t *buffer;
    int bufferCount;
    int out_sampleSize;
    int out_channels;
    int out_sampleRate;

};


#endif //LAPUTA_FFMPEG_AUDIOCHANNEL_H
