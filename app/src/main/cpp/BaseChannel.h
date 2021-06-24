//
// Created by Administrator on 2019/11/14.
//

#ifndef PLAYER_BASECHANNEL_H
#define PLAYER_BASECHANNEL_H

#include "safe_queue.h"
#include "JavaCallHelper.h"
#include "Log.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

class BaseChannel {
public:
    BaseChannel(int channelId, JavaCallHelper *helper, AVCodecContext *avCodecContext, AVRational
    base) : channelId(channelId),
            helper(helper),
            avCodecContext(avCodecContext),
            time_base(base) {
        pkt_queue.setReleaseHandle(releaseAvPacket);
        frame_queue.setReleaseHandle(releaseAvFrame);
    }


    virtual ~BaseChannel() {
        if (avCodecContext) {
            avcodec_close(avCodecContext);
            avcodec_free_context(&avCodecContext);
            avCodecContext = 0;
        }
        pkt_queue.clear();
        frame_queue.clear();
    };

    virtual void play() =0 ;

    virtual void stop()=0;

    virtual void decode()=0 ;

    void setEnable(bool enable) {
        pkt_queue.setEnable(enable);
        frame_queue.setEnable(enable);
    }

    static void releaseAvFrame(AVFrame *&frame) {
        if (frame) {
            av_frame_free(&frame);
            frame = 0;
        }
    }


    static void releaseAvPacket(AVPacket *&packet) {
        if (packet) {
            av_packet_free(&packet);
            packet = 0;
        }
    }


public:
    int channelId;
    JavaCallHelper *helper;
    AVCodecContext *avCodecContext;
    AVRational time_base;

    SafeQueue<AVPacket *> pkt_queue; // 解码前的结构体
    SafeQueue<AVFrame *> frame_queue; // 解码后播放的结构体
    bool isPlaying = false;

    double clock = 0;

};

#endif //PLAYER_BASECHANNEL_H
