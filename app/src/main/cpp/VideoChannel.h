//
// Created by xpl on 2021/6/23.
//

#ifndef LAPUTA_FFMPEG_VIDEOCHANNEL_H
#define LAPUTA_FFMPEG_VIDEOCHANNEL_H

#include <android/native_window.h>
#include "BaseChannel.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

class VideoChannel : public BaseChannel {
    friend void *doDecode_t(void *args);

    friend void *doPlay_t(void *args);

public:
    VideoChannel(int channelId, JavaCallHelper *helper, AVCodecContext *avCodecContext,
                 const AVRational &base, int fps);

    virtual ~VideoChannel();

public:
    virtual void play();

    virtual void stop();

    virtual void decode();

    void setWindow(ANativeWindow *pWindow);

private:
    void realPlay();
    void onDraw(uint8_t *pString[4], int pInt[4], int width, int height);

private:
    int fps;
    int isPlaying = 0;
    pthread_t videoDecodeTask = 0;
    pthread_t videoPlayTask = 0;
    ANativeWindow *aNativeWindow = 0;
    pthread_mutex_t windowMutex ;


};


#endif //LAPUTA_FFMPEG_VIDEOCHANNEL_H