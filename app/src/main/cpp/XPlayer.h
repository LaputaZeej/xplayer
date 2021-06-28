//
// Created by xpl on 2021/6/23.
//

#ifndef LAPUTA_FFMPEG_XPLAYER_H
#define LAPUTA_FFMPEG_XPLAYER_H

#include <pthread.h>
#include "JavaCallHelper.h"
#include "VideoChannel.h"
#include <android/native_window_jni.h>
#include "AudioChannel.h"

extern "C" {
#include <libavformat/avformat.h>
}

class XPlayer {
    friend void *doPrepare(void *args);

    friend void *doStart(void *args);

public:
    XPlayer(JavaCallHelper *helper);

    ~XPlayer();

public:
    void setDataSource(const char *path_);

    void setWindow(ANativeWindow *window);

    void prepare();

    void start();

    void stop();

    void release();

private:
    char *path;
    JavaCallHelper *javaCallHelper = 0;
    pthread_t prepareTask=0;
    int64_t duration;
    VideoChannel *videoChannel = 0;
    int isPlaying = 0;
    pthread_t startTask;

    AVFormatContext *avFormatContext;
    ANativeWindow *aNativeWindow;

    AudioChannel *audioChannel=0;

public:
    int aFormat;
    int aWidth;
    int aHeight;


private:
    void realPrepare();

    void realStart();
};


#endif //LAPUTA_FFMPEG_XPLAYER_H
