#include <jni.h>
#include <string>
#include "XPlayer.h"

JavaVM *javaVm = 0;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVm = vm;
    return JNI_VERSION_1_4;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_laputa_ffmpeg_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_laputa_ffmpeg_XPlayer_nativeInit(JNIEnv *env, jobject thiz) {
    XPlayer *xPlayer = new XPlayer(new JavaCallHelper(javaVm, env, thiz));
    return reinterpret_cast<jlong>(xPlayer);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_laputa_ffmpeg_XPlayer_nativeSetDataSource(JNIEnv *env, jobject thiz, jlong pointer,
                                                   jstring path) {
    const char *_path = env->GetStringUTFChars(path, 0);
    XPlayer *xPlayer = reinterpret_cast<XPlayer *>(pointer);
    xPlayer->setDataSource(_path);
    env->ReleaseStringUTFChars(path, _path);
}extern "C"
JNIEXPORT void JNICALL
Java_com_laputa_ffmpeg_XPlayer_nativeSetPrepare(JNIEnv *env, jobject thiz, jlong pointer) {
    XPlayer *xPlayer = reinterpret_cast<XPlayer *>(pointer);
    xPlayer->prepare();
}extern "C"
JNIEXPORT void JNICALL
Java_com_laputa_ffmpeg_XPlayer_nativeStart(JNIEnv *env, jobject thiz, jlong pointer) {
    XPlayer *xPlayer = reinterpret_cast<XPlayer *>(pointer);
    xPlayer->start();
}extern "C"
JNIEXPORT void JNICALL
Java_com_laputa_ffmpeg_XPlayer_nativeStop(JNIEnv *env, jobject thiz, jlong pointer) {

}extern "C"
JNIEXPORT void JNICALL
Java_com_laputa_ffmpeg_XPlayer_nativeSetSurface(JNIEnv *env, jobject thiz, jlong pointer,
                                                jobject surface) {
    XPlayer *xPlayer = reinterpret_cast<XPlayer *>(pointer);
    ANativeWindow *window = ANativeWindow_fromSurface(env,surface);
    xPlayer->setWindow(window);

}