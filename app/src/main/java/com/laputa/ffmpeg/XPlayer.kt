package com.laputa.ffmpeg

import android.view.Surface
import java.lang.IllegalStateException

/**
 * Author by xpl, Date on 2021/6/23.
 */
class XPlayer {
    var onErrorCallback: (Int) -> Unit = {}
    var onPrepareCallback: () -> Unit = {}
    var onProgressCallback: (Int) -> Unit = {}

    private var nativePointer: Long = -1L

    init {
        nativePointer = nativeInit()
    }

    fun setDataSource(path: String) {
        if (nativePointer == -1L) {
            throw IllegalStateException("XPlayer初始化失败")
        }
        nativeSetDataSource(nativePointer, path)
    }

    fun setSurface(surface: Surface, format: Int, width: Int, height: Int) {
        nativeSetSurface(nativePointer, surface)
//        nativeSetSurface2(nativePointer, surface,format,width,height)
    }

    fun prepare() {
        nativeSetPrepare(nativePointer)
    }

    fun start() {
        nativeStart(nativePointer)
    }

    fun stop() {
        nativeStop(nativePointer)
    }

    private external fun nativeInit(): Long
    private external fun nativeSetDataSource(pointer: Long, path: String)
    private external fun nativeSetPrepare(pointer: Long)
    private external fun nativeStart(pointer: Long)
    private external fun nativeStop(pointer: Long)
    private external fun nativeSetSurface(pointer: Long, surface: Surface)
    private external fun nativeSetSurface2(pointer: Long, surface: Surface, format: Int, width: Int, height: Int)


    private fun onError(code: Int) {
        onErrorCallback(code)
    }

    private fun onPrepare() {
        onPrepareCallback()
    }

    private fun onProgress(progress: Int) {
        onProgressCallback(progress)
    }

    companion object {
        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}