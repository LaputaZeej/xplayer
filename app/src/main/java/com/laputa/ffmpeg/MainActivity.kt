package com.laputa.ffmpeg

import android.os.Bundle
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.WindowManager
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {

    private var xPlayer: XPlayer? = createPlayer()

    private fun createPlayer(): XPlayer {
        return XPlayer().apply {
            onPrepareCallback = {
                runOnUiThread {
                    Toast.makeText(this@MainActivity, "prepare ok", Toast.LENGTH_SHORT).show()
                }
                this.start()
            }

            onErrorCallback = {
                runOnUiThread {
                    Toast.makeText(this@MainActivity, "error$it", Toast.LENGTH_SHORT).show()
                }
            }

            onProgressCallback = {

            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setFlags(
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
        )
        setContentView(R.layout.activity_main)
        // Example of a call to a native method
        findViewById<TextView>(R.id.sample_text).setOnClickListener {
            xPlayer?.setDataSource(getPath())
            xPlayer?.prepare()
        }
        findViewById<TextView>(R.id.sample_text_local).setOnClickListener {
            xPlayer?.setDataSource(application.filesDir.absolutePath + "/c.mp4")
            xPlayer?.prepare()
        }

        findViewById<TextView>(R.id.sample_text_stop).setOnClickListener {
            xPlayer?.stop()
            xPlayer = createPlayer()
        }

//        xPlayer.setDataSource(getPath())
//        xPlayer.prepare()

        val surfaceView = findViewById<SurfaceView>(R.id.surface)
        surfaceView.holder.addCallback(object : SurfaceHolder.Callback2 {
            override fun surfaceCreated(holder: SurfaceHolder?) {
            }

            override fun surfaceChanged(
                holder: SurfaceHolder?,
                format: Int,
                width: Int,
                height: Int
            ) {
                holder?.surface?.run {
                    xPlayer?.setSurface(this)
                }
            }

            override fun surfaceDestroyed(holder: SurfaceHolder?) {
            }

            override fun surfaceRedrawNeeded(holder: SurfaceHolder?) {
            }

        })

    }

    override fun onDestroy() {
        super.onDestroy()
        xPlayer?.stop()
        xPlayer = null
    }

    private fun getPath() =
        "http://ivi.bupt.edu.cn/hls/cctv1hd.m3u8"
    //application.filesDir.absolutePath + "/c.mp4"




}