//
// Created by xpl on 2021/6/23.
//


#include "VideoChannel.h"
#include <SLES/OpenSLES_Android.h>

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/rational.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

#define AV_SYNC_THRESHOLD_MIN 0.04
#define AV_SYNC_THRESHOLD_MAX 0.1


VideoChannel::VideoChannel(int channelId, JavaCallHelper *helper, AVCodecContext *avCodecContext,
                           const AVRational &base, double fps) :
        BaseChannel(channelId, helper, avCodecContext, base), fps(fps) {
    LOGI("VideoChannel::VideoChannel p = %p,thread_count = %d", &avCodecContext,
         avCodecContext->thread_count);
    pthread_mutex_init(&windowMutex, 0);
}

VideoChannel::~VideoChannel() {
    pthread_mutex_destroy(&windowMutex);
}

void *doDecode_t(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->decode();
    return 0;
}

void *doPlay_t(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->realPlay();
    return 0;
}

void VideoChannel::play() {
    isPlaying = 1;
    setEnable(true); //
    // 解码 线程
    pthread_create(&videoDecodeTask, 0, doDecode_t, this);
    // 播放 线程
    pthread_create(&videoPlayTask, 0, doPlay_t, this);
}

void VideoChannel::decode() {
    LOGI("VideoChannel::decode p = %p,thread_count = %d", &avCodecContext,
         avCodecContext->thread_count);
    // 从队列中取包
    AVPacket *packet = 0;
    while (isPlaying) {
        // 阻塞
        // 1.有数据2.停止播放
        int ret = pkt_queue.deQueue(packet);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        // 向解码器发送解码数据
        ret = avcodec_send_packet(avCodecContext, packet);
        releaseAvPacket(packet);// pack释放
//        if(ret==EAGAIN){ // frame满了
//
//        }
        if (ret < 0) {
            break;
        }
        // 从解码器中取出解码好的数据
        AVFrame *avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, avFrame);
        if (ret == AVERROR(EAGAIN)) { // 我还要,解码 i p b
            if (avFrame) {
                releaseAvFrame(avFrame);
            }
            continue;
        } else if (ret < 0) {
            if (avFrame) {
                releaseAvFrame(avFrame);
            }
            break;
        }

        LOGI("VideoChannel::decode 成功 ");
        //todo 这里还有音频的，更好的实现是需要时再解码，只需要一个线程与一个待解码队列
//        while (frame_queue.size() > fps * 10 && isPlaying) {
//            av_usleep(1000 * 20);
//        }
        // 放入带播放的队列
        frame_queue.enQueue(avFrame);
    }
    releaseAvPacket(packet);
    LOGI("VideoChannel::decode 结束 ");
}

void VideoChannel::realPlay() {
    LOGI("VideoChannel::realPlay aWidth=%d,aHeight=%d", aWidth, aHeight);
//    int w_width = aWidth/*avCodecContext->width*/;
//    int w_height = aHeight/*avCodecContext->height*/;
    int w_width = avCodecContext->width;
    int w_height = avCodecContext->height;
    // 用来缩放&格式转换
    SwsContext *swsContext = sws_getContext(
            avCodecContext->width,
            avCodecContext->height,
            /*色彩空间格式*/
            avCodecContext->pix_fmt,
            w_width,
            w_height,
            AV_PIX_FMT_RGBA, SWS_FAST_BILINEAR, 0, 0, 0);
    AVFrame *avFrame = 0;
    double frame_delay = 1.0 / fps;
    int ret;
    uint8_t *data[4];
    int lineSize[4];
    // 计算
    av_image_alloc(data, lineSize, w_width, w_height, AV_PIX_FMT_RGBA,
                   1);
    while (isPlaying) {
        ret = frame_queue.deQueue(avFrame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }

        double extra_delay = (avFrame->repeat_pict * 1.0) / (2 * fps);
        double delay = (extra_delay + frame_delay);
        LOGI("      VideoChannel::realPlay DELAY=%lf", delay);
        if (audioChannel) {
//            avFrame->pts*av_q2d(time_base);
            // best_effort_timestamp = pts ijkplayer
            clock = avFrame->best_effort_timestamp * av_q2d(time_base);
            double offset = clock - audioChannel->clock;
            // 允许范围 AV_SYNC_THRESHOLD_MIN 0.04 ~ AV_SYNC_THRESHOLD_MAX 0.1
            // https://github.com/bilibili/ijkplayer/blob/master/ijkmedia/ijkplayer/ff_ffplay_def.h

            double sync = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
            if (offset <= -sync) {
                // 视频落后太多，让delay减少
                delay = FFMAX(0.0, delay + offset);
            } else if (offset > sync) {
                // 视频快了
                delay = delay + offset;
            }

            if (offset <= -AV_SYNC_THRESHOLD_MAX) {
                audioChannel->updateDelay(-offset * AV_TIME_BASE);
            } else {
                audioChannel->updateDelay(0);
            }
            LOGI("      VideoChannel::realPlay V=%lf,A=%lf,A-V=%lf DELAY=%lf", clock,
                 audioChannel->clock,
                 -offset, delay);
        }


        av_usleep(delay * AV_TIME_BASE);

        // ANativeWindow 只能显示RGBA...
        // libswscale
        // ijkPlayer google libyuv


        // R []
        // G []
        // B []
        // A []
        sws_scale(swsContext,
                /*元数据 指针数组*/
                  avFrame->data,
                /*每一行的数据个数*/
                  avFrame->linesize,
                /*offset*/
                  0,
                  avFrame->height,
                  data,
                  lineSize
        );
        onDraw(data, lineSize, w_width, w_height);
        releaseAvFrame(avFrame);
    }
    // av_free(&data[0]);
    isPlaying = 0;
    if (avFrame) {
        releaseAvFrame(avFrame);
    }
    sws_freeContext(swsContext);
    LOGI("VideoChannel::realPlay end");
}


void VideoChannel::onDraw(uint8_t **data, int *lineSize, int width, int height) {
    pthread_mutex_lock(&windowMutex);
    if (!aNativeWindow) {
        pthread_mutex_unlock(&windowMutex);
        return;
    }

    // aNativeWindow也有宽高,不是物理宽高，是像素宽高，这里为原始数据宽高
    ANativeWindow_setBuffersGeometry(aNativeWindow, width, height, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(aNativeWindow, &buffer, 0)) {
        ANativeWindow_release(aNativeWindow);
        aNativeWindow = 0;
        pthread_mutex_unlock(&windowMutex);
        return;
    }
    // 处理数据 把视频数据刷到buff中，字节对齐
    // 64字节对齐 uint16_t reserved; // 无意义 占位用
    uint8_t *dstData = static_cast<uint8_t *>(buffer.bits);
    int dstSize = buffer.stride * 4; // 852*4

    uint8_t *srcData = data[0]; // 经过sws_scale，数据都转到了data[0]，铺平了
    int srcSize = lineSize[0];  // 864*4

    //一行行拷贝
    // 4*4 8  window 8字节对齐
    // 4*4 4  ffmpeg 4字节对齐
    // xxxxxxxx
    // xxxxxxxx
    // xxxxxxxx
    // xxxxxxxx
    for (int i = 0; i < buffer.height; ++i) {
        memcpy(dstData + i * dstSize, srcData + i * srcSize, srcSize);
    }
    //LOGI("VideoChannel::onDraw %p", aNativeWindow);
    ANativeWindow_unlockAndPost(aNativeWindow);
    pthread_mutex_unlock(&windowMutex);
}

void VideoChannel::setWindow(ANativeWindow *pWindow) {
    // ANativeWindow
    // C:\sdk\ndk\20.0.5594570\platforms\android-24\arch-arm\usr\lib
    // 枷锁
    pthread_mutex_lock(&windowMutex);
    if (this->aNativeWindow) {
        ANativeWindow_release(this->aNativeWindow);
    }
    this->aNativeWindow = pWindow;
    pthread_mutex_unlock(&windowMutex);

}

void VideoChannel::stop() {
    isPlaying = 0;
    helper = 0;
    setEnable(0);
    pthread_join(videoDecodeTask, 0);
    pthread_join(videoPlayTask, 0);
    if (aNativeWindow) {
        ANativeWindow_release(aNativeWindow);
        aNativeWindow = 0;
    }
}








