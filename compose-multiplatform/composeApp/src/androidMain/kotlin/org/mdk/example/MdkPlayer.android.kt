package org.mdk.example

import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.viewinterop.AndroidView

// ─────────────────────────────────────────────────────────────────────────────
// MdkPlayer – Android actual
//
// Thin Kotlin wrapper around the JNI functions defined in MdkPlayerJNI.cpp.
// The MDK C++ Player is constructed natively; its pointer is kept in [handle].
// ─────────────────────────────────────────────────────────────────────────────

actual class MdkPlayer actual constructor() {

    companion object {
        init {
            // libmdk.so must be loaded first (it is the MDK SDK shared library).
            // mdk_player_jni.so is the thin JNI wrapper built from MdkPlayerJNI.cpp.
            System.loadLibrary("mdk")
            System.loadLibrary("mdk_player_jni")
        }
    }

    private val handle: Long = nativeCreate()

    actual fun setMedia(url: String) = nativeSetMedia(handle, url)
    actual fun play()                = nativePlay(handle)
    actual fun pause()               = nativePause(handle)
    actual fun stop()                = nativeStop(handle)

    actual fun release() {
        // Detach any surface before destroying the player
        nativeUpdateSurface(handle, null, 0, 0)
        nativeRelease(handle)
    }

    /** Attach (or detach) a Surface as the MDK render target. */
    fun updateSurface(surface: Surface?, width: Int, height: Int) =
        nativeUpdateSurface(handle, surface, width, height)

    /** Notify MDK that the surface dimensions changed. */
    fun setSurfaceSize(width: Int, height: Int) =
        nativeSetSurfaceSize(handle, width, height)

    // ── JNI declarations ──────────────────────────────────────────────────

    private external fun nativeCreate(): Long
    private external fun nativeRelease(handle: Long)
    private external fun nativeSetMedia(handle: Long, url: String)
    private external fun nativePlay(handle: Long)
    private external fun nativePause(handle: Long)
    private external fun nativeStop(handle: Long)
    private external fun nativeUpdateSurface(handle: Long, surface: Surface?,
                                              width: Int, height: Int)
    private external fun nativeSetSurfaceSize(handle: Long, width: Int, height: Int)
}

// ─────────────────────────────────────────────────────────────────────────────
// VideoSurface – Android actual
//
// Embeds a SurfaceView via AndroidView.  The SurfaceHolder callbacks forward
// surface lifecycle events to the MDK player.
// ─────────────────────────────────────────────────────────────────────────────

@Composable
actual fun VideoSurface(player: MdkPlayer, modifier: Modifier) {
    AndroidView(
        modifier = modifier,
        factory = { context ->
            SurfaceView(context).also { view ->
                view.holder.addCallback(object : SurfaceHolder.Callback {
                    override fun surfaceCreated(holder: SurfaceHolder) {
                        val w = view.width.takeIf { it > 0 } ?: 1
                        val h = view.height.takeIf { it > 0 } ?: 1
                        player.updateSurface(holder.surface, w, h)
                    }

                    override fun surfaceChanged(
                        holder: SurfaceHolder,
                        format: Int,
                        width: Int,
                        height: Int,
                    ) {
                        player.setSurfaceSize(width, height)
                    }

                    override fun surfaceDestroyed(holder: SurfaceHolder) {
                        player.updateSurface(null, 0, 0)
                    }
                })
            }
        },
    )
}
