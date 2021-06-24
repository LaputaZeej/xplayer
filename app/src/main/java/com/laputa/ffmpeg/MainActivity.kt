package com.laputa.ffmpeg

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.TextView
import android.widget.Toast
import kotlin.random.Random

class MainActivity : AppCompatActivity() {

    private val xPlayer: XPlayer = XPlayer().apply {
        onPrepareCallback = {
            runOnUiThread {
                Toast.makeText(this@MainActivity, "prepare ok", Toast.LENGTH_SHORT).show()
            }

            //this.stop()
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

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Example of a call to a native method
        findViewById<TextView>(R.id.sample_text).text = stringFromJNI()
        findViewById<TextView>(R.id.sample_text).setOnClickListener {
            xPlayer.setDataSource(getPath())
            xPlayer.prepare()
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
                    xPlayer.setSurface(this)
                }
            }

            override fun surfaceDestroyed(holder: SurfaceHolder?) {
            }

            override fun surfaceRedrawNeeded(holder: SurfaceHolder?) {
            }

        })

    }

    private fun getPath() =
        application.filesDir.absolutePath + "/a.mp4"

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {
        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}