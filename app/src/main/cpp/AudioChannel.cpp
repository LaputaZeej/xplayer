//
// Created by xpl on 2021/6/24.
//

#include "AudioChannel.h"
#include "Log.h"
extern "C"{
#include <libavutil/time.h>
}

AudioChannel::AudioChannel(int channelId, JavaCallHelper *helper, AVCodecContext *avCodecContext,
                           const AVRational &base) : BaseChannel(channelId, helper, avCodecContext,
                                                                 base) {
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO); //2
    // 采样位
    out_sampleSize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    // 采样率
    out_sampleRate = 44100;
    bufferCount = out_sampleRate * out_sampleSize * out_channels;
    // 计算转换后数据的最大字节数
    buffer = static_cast<uint8_t *>(malloc(bufferCount));
}

AudioChannel::~AudioChannel() {
    free(buffer);
    buffer = 0;
    LOGI("~AudioChannel");
}

void *audio_Decode_t(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->realDecode();
    return 0;
}

void *audio_Play_t(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->realPlay();
    return 0;
}

void AudioChannel::play() {
    // 转换器
    swrContext = swr_alloc_set_opts(0,
            /*双声道*/
                                    AV_CH_LAYOUT_STEREO,
            /*采样位*/
                                    AV_SAMPLE_FMT_S16,
            /*采样率*/
                                    44100,
                                    avCodecContext->channel_layout,
                                    avCodecContext->sample_fmt,
                                    avCodecContext->sample_rate, 0, 0
    );
    swr_init(swrContext);

    isPlaying = 1;
    setEnable(1);
    // 解码
    pthread_create(&audioDecodeTask, 0, audio_Decode_t, this);
    // 播放
    pthread_create(&audioPlayTask, 0, audio_Play_t, this);
}

void AudioChannel::stop() {
    isPlaying = 0;
    helper = 0;
    setEnable(0);
    pthread_join(audioDecodeTask, 0);
    pthread_join(audioPlayTask, 0);

    releaseOpenSL();
    if (swrContext) {
        swr_free(&swrContext);
        swrContext = 0;
    }
}

void AudioChannel::decode() {
    LOGI("AudioChannel::decode ");
    AVPacket *avPacket = 0;
    int ret = 0;
    while (isPlaying) {
        ret = pkt_queue.deQueue(avPacket);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        ret = avcodec_send_packet(avCodecContext, avPacket);
        if (ret < 0) {
            break;
        }
        AVFrame *avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, avFrame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            break;
        }
        while (frame_queue.size() > 100 && isPlaying) {
            av_usleep(1000 * 20);
        }
        frame_queue.enQueue(avFrame);
    }
}

void AudioChannel::realDecode() {
    decode();
}

/**
 * 播放回调
 */
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf caller,
                      void *pContext) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(pContext);
    int dataSize = audioChannel->getData();
    if (dataSize > 0) {
        (*caller)->Enqueue(caller, audioChannel->buffer, dataSize);
    }
}

int AudioChannel::getData() {
    int dataSize = 0;
    AVFrame *avFrame = 0;
    while (isPlaying) {
        int ret = frame_queue.deQueue(avFrame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        // 转换
        int nb = swr_convert(swrContext, /*out*/&buffer, bufferCount,
                /*in*/(const uint8_t **) (avFrame->data),/*样本数*/avFrame->nb_samples);
        dataSize = nb * out_channels * out_sampleSize;
        // 音频的时钟
        clock = avFrame->pts * av_q2d(time_base);
        LOGI("AudioChannel::getData %d clock=%lf", nb, clock);
        break;
    }
    releaseAvFrame(avFrame);
    return dataSize;
}

void AudioChannel::realPlay() {
    LOGI("AudioChannel::realPlay");
    /**
     * 创建引擎
     */
    // 1.创建引擎对象
    // SLObjectItf slObjectItf = NULL;
    SLresult result;
    result = slCreateEngine(&slObjectItf, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 2. 初始化
    result = (*slObjectItf)->Realize(slObjectItf,/*同步*/SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 3. 获取引擎接口
    //SLEngineItf engineItf = NULL;
    result = (*slObjectItf)->GetInterface(slObjectItf, SL_IID_ENGINE, &engineItf);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    /**
     * 创建混音器
     */
    // 1.通过引擎接口创建混音器
    // SLObjectItf outputMixObject = NULL;
    result = (*engineItf)->CreateOutputMix(engineItf, &outputMixObject, 0, 0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 2.初始化混音器
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    /**
     * 创建播放器
     */
    //数据源 （数据获取器+格式） avFrame
    // PCM
    //      声道   单声道/双声道
    //      采样率  44k
    //      采样位   16位 32位
    // 重采样 pcm格式
    //创建buffer缓冲类型的队列作为数据定位器（获取播放数据） 2个缓冲区
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    //pcm数据格式: pcm、声道数、采样率、采样位、容器大小、通道掩码(双声道)、字节序(小端)
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource slDataSource = {&android_queue, &pcm};
    // 设置混音器，真正播放的
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, NULL};
    // 需要的接口
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE}; // 队列操作接口
    const SLboolean req[1] = {SL_BOOLEAN_TRUE}; // 是否必须
    // SLObjectItf bqPlayerObject = NULL;
    // 创建播放器，对混音器的包装，比如开始停止等
    (*engineItf)->CreateAudioPlayer(engineItf, &bqPlayerObject, &slDataSource, &audioSnk, 1, ids,
                                    req);
    //初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    //获得播放数据队列操作接口
    // SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue = NULL;
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue);
    //设置回调（启动播放器后执行回调来获取数据并播放）
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);
    //获取播放状态接口
    //SLPlayItf bqPlayerInterface = NULL;
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerInterface);
    // 设置播放状态
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);

    // 手动调用一次，才能播放
    bqPlayerCallback(bqPlayerBufferQueue, this);
}

void AudioChannel::releaseOpenSL() {
    LOGE("停止播放");
    //设置停止状态
    if (bqPlayerInterface) {
        (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_STOPPED);
        bqPlayerInterface = 0;
    }
    //销毁播放器
    if (bqPlayerObject) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = 0;
        bqPlayerBufferQueue = 0;
    }
    //销毁混音器
    if (outputMixObject) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = 0;
    }
    //销毁引擎
    if (slObjectItf) {
        (*slObjectItf)->Destroy(slObjectItf);
        slObjectItf = 0;
        engineItf = 0;
    }
}


