
#ifndef ENJOYPLAYER_LOG_H
#define ENJOYPLAYER_LOG_H

#include <android/log.h>

#define  LOGE(...)    __android_log_print(ANDROID_LOG_ERROR,"laputa_FFMPEG",__VA_ARGS__)

#define  LOGI(...)    __android_log_print(ANDROID_LOG_INFO,"laputa_FFMPEG",__VA_ARGS__)

#endif //ENJOYPLAYER_LOG_H
