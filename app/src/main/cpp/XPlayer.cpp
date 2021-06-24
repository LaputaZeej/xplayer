//
// Created by xpl on 2021/6/23.
//

#include "XPlayer.h"
#include <malloc.h>
#include <cstring>
#include <chrono>
#include "Log.h"


XPlayer::XPlayer(JavaCallHelper *helper) : javaCallHelper(helper) {
    avformat_network_init();
    videoChannel = nullptr;
}

XPlayer::~XPlayer() {

}

void XPlayer::setDataSource(const char *path_) {
    // 深拷贝操作 不能直接赋值 可能在其他地方回收
    int size = strlen(path_);

    // c
//    this->path = static_cast<char *>(malloc(size + 1)); // C "\0"
//    memset((void *) this->path, 0, size + 1);
//    memcpy(this->path, path_, size);

    // c++
    this->path = new char[size + 1];
    strcpy(this->path, path_);
}

void *doPrepare(void *args) {
    XPlayer *xPlayer = static_cast<XPlayer *>(args);
    // avformat_open_input(0, xPlayer->path, 0, 0); // 友元函数,拿取私有属性
    xPlayer->realPrepare();
    return 0;
}

void XPlayer::prepare() {
    // 解析 耗时 开启线程
    pthread_create(&prepareTask, 0, doPrepare, this);
}

void XPlayer::realPrepare() {
    LOGI("XPlayer::realPrepare");
    // 1.打开媒体文件
    avFormatContext = avformat_alloc_context();
//    AVDictionary  *opts;
//    av_dict_set(&opts,"timeout","3000000",0);
    int ret = avformat_open_input(
            /*传入的是指针的指针*/
            &avFormatContext,
            path,
            /*输入文件的封装格式avi flv，自动识别0*/
            0,
            /*map集合，打开网络文件*/
            0);
    if (ret != 0) {
        char *error = av_err2str(ret);
        LOGE("打开%s失败[%d]%s", path, ret, error);
        javaCallHelper->onError(FFMPEG_CAN_NOT_OPEN_URL, THREAD_CHILD);
        return;
    }

    // 2.获取媒体流信息
    ret = avformat_find_stream_info(avFormatContext, 0);
    if (ret < 0) {
        LOGE("获取媒体流信息%s失败[%d]%s", path, ret, av_err2str(ret));
        javaCallHelper->onError(FFMPEG_CAN_NOT_FIND_STREAMS, THREAD_CHILD);
        return;
    }
    duration = avFormatContext->duration / AV_TIME_BASE;  // 时长
    // 几个流
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        AVStream *avStream = avFormatContext->streams[i];
        // 解码信息
        AVCodecParameters *avCodecParameters = avStream->codecpar;
        // 查找解码器
        AVCodec *avCodec = avcodec_find_decoder(
                avCodecParameters->codec_id);     // ffmpeg -decoders 查看ffmpeg支持编码格式
        if (!avCodec) {
            javaCallHelper->onError(FFMPEG_FIND_DECODER_FAIL, THREAD_CHILD);
            return;
        }
        // 初始化解码器
        AVCodecContext *avCodecContext = avcodec_alloc_context3(avCodec); // 初始化
        ret = avcodec_parameters_to_context(avCodecContext,
                                            avCodecParameters); // 把解码信息赋值给新初始化的上下文（不做改变）,也可以自己设置参数
        if (ret < 0) {
            javaCallHelper->onError(FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL, THREAD_CHILD);
            return;
        }
        // 打开解码器
        if (avcodec_open2(avCodecContext, avCodec, 0) != 0) {
            javaCallHelper->onError(FFMPEG_OPEN_DECODER_FAIL, THREAD_CHILD);
            return;
        }
        AVMediaType avMediaType = avCodecParameters->codec_type;


        if (avMediaType == AVMEDIA_TYPE_AUDIO) {
            LOGI("      XPlayer::realPrepare 音频channel");
            audioChannel = new AudioChannel(i, javaCallHelper, avCodecContext, avStream->time_base);

        } else if (avMediaType == AVMEDIA_TYPE_VIDEO) {
            LOGI("      XPlayer::realPrepare 视频channel");
            // 帧率 1s 多少图片
            int fps = av_q2d(avStream->avg_frame_rate);
            //
            videoChannel = new VideoChannel(i, javaCallHelper, avCodecContext, avStream->time_base,
                                            fps);
            videoChannel->setWindow(aNativeWindow);
        }

    }

    // 没有视频&&没有音频
    if (!videoChannel && !audioChannel) {
        javaCallHelper->onError(FFMPEG_NOMEDIA, THREAD_CHILD);
        return;
    }
    LOGI("XPlayer::realPrepare ok!");
    // 准备好
    javaCallHelper->onParpare(THREAD_CHILD);


}

void *doStart(void *args) {
    XPlayer *xPlayer = static_cast<XPlayer *>(args);
    xPlayer->realStart();
    return 0;
}

void XPlayer::start() {
    // 1. 读取媒体源的数据
    // 2.根据数据类型放入AudioChannel/VideoChannel队列中
    isPlaying = 1;
    if (videoChannel) {
        videoChannel->play();
    }
    if (audioChannel) {
        audioChannel->play();
    }
    pthread_create(&startTask, 0, doStart, this);
}

void XPlayer::realStart() {
    LOGI("XPlayer::realStart");
    while (isPlaying) {
        AVPacket *packet = av_packet_alloc();
        // 从流中拿packet
        int ret = av_read_frame(avFormatContext, packet);
        if (ret == 0) {
            if (videoChannel && packet->stream_index == videoChannel->channelId) {
                LOGI("      XPlayer::realStart 视频包");
                videoChannel->pkt_queue.enQueue(packet);
            } else if (audioChannel && packet->stream_index == audioChannel->channelId) {
                LOGI("      XPlayer::realStart 音频包");
                audioChannel->pkt_queue.enQueue(packet);
            } else {
                if (packet) {
                    av_packet_free(&packet);
                    packet = 0;
                }
            }
        } else if (ret == AVERROR_EOF) { // 文件结尾 end of file
            // 读取完毕不一定播放完毕
            if (videoChannel&&videoChannel->pkt_queue.empty() && videoChannel->frame_queue.empty()
                && audioChannel && audioChannel->pkt_queue.empty() && audioChannel->frame_queue.empty()
                    ) {
                // 播放完毕
                LOGI("XPlayer::realStart 播放完毕");
                break;
            }
        } else {
            LOGE("XPlayer::realStart 读取数据包失败，返回:%d 错误描述:%s", ret, av_err2str(ret));
            break;
        }
    }
    isPlaying = 0;
    if (videoChannel) {
        videoChannel->stop();
    }
    if (audioChannel) {
        audioChannel->stop();
    }
}

void XPlayer::setWindow(ANativeWindow *window) {
    this->aNativeWindow = window;

    if (videoChannel) {
        videoChannel->setWindow(window);
    }
}


