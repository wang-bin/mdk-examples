package org.mdk.example

import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier

/**
 * Platform-agnostic MDK player handle.
 *
 * Each platform provides an `actual` implementation that wraps the native
 * MDK SDK via the most appropriate mechanism:
 *  - iOS   : Kotlin/Native C interop → MDK C API (mdk/c/Player.h)
 *  - Android: JNI → thin C++ wrapper around MDK C++ API (mdk/Player.h)
 *  - Desktop: JNA → pre-built mdk_shim shared library (see desktopMain/native)
 */
expect class MdkPlayer() {
    /** Set the media URL or file path to play. */
    fun setMedia(url: String)

    /** Start or resume playback. */
    fun play()

    /** Pause playback. */
    fun pause()

    /** Stop playback. */
    fun stop()

    /** Release all native resources. Call when the player is no longer needed. */
    fun release()
}

/**
 * A Composable that provides the native rendering surface for [player].
 *
 * On iOS   this embeds a UIView whose backing layer is passed to MDK.
 * On Android this embeds a SurfaceView and forwards its Surface to MDK.
 * On Desktop this embeds an AWT Canvas whose native handle is passed to MDK.
 */
@Composable
expect fun VideoSurface(player: MdkPlayer, modifier: Modifier = Modifier)
