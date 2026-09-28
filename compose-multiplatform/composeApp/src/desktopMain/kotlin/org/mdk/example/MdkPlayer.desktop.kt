package org.mdk.example

import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.awt.SwingPanel
import com.sun.jna.Library
import com.sun.jna.Native
import com.sun.jna.Pointer
import java.awt.Canvas

// ─────────────────────────────────────────────────────────────────────────────
// MdkShim – JNA interface for the pre-built mdk_shim native library
//
// The mdk_shim library wraps the MDK C API struct's function pointers into
// plain exported C functions that JNA can call directly (no manual struct
// offset arithmetic required).
//
// Build instructions: see desktopMain/native/CMakeLists.txt
// The resulting library must be on java.library.path at runtime:
//   Linux   : libmdk_shim.so
//   macOS   : libmdk_shim.dylib
//   Windows : mdk_shim.dll
// ─────────────────────────────────────────────────────────────────────────────

private interface MdkShim : Library {
    fun mdk_shim_create(): Pointer?
    fun mdk_shim_destroy(api: Pointer)
    fun mdk_shim_set_media(api: Pointer, url: String)
    fun mdk_shim_play(api: Pointer)
    fun mdk_shim_pause(api: Pointer)
    fun mdk_shim_stop(api: Pointer)
    fun mdk_shim_set_volume(api: Pointer, volume: Float)
    fun mdk_shim_set_playback_rate(api: Pointer, rate: Float)
    fun mdk_shim_update_native_surface(api: Pointer, surface: Pointer?, width: Int, height: Int)
    fun mdk_shim_set_surface_size(api: Pointer, width: Int, height: Int)
    fun mdk_shim_render_video(api: Pointer): Int
    fun mdk_shim_seek(api: Pointer, positionMs: Long)
    fun mdk_shim_position(api: Pointer): Long
    fun mdk_shim_set_global_option(key: String, value: String)
}

private val shim: MdkShim by lazy {
    Native.load("mdk_shim", MdkShim::class.java)
}

// ─────────────────────────────────────────────────────────────────────────────
// MdkPlayer – Desktop actual
// ─────────────────────────────────────────────────────────────────────────────

actual class MdkPlayer actual constructor() {

    private val api: Pointer = shim.mdk_shim_create()
        ?: error("mdk_shim_create() returned null – is mdk_shim on java.library.path?")

    actual fun setMedia(url: String) = shim.mdk_shim_set_media(api, url)
    actual fun play()                = shim.mdk_shim_play(api)
    actual fun pause()               = shim.mdk_shim_pause(api)
    actual fun stop()                = shim.mdk_shim_stop(api)

    actual fun release() {
        shim.mdk_shim_update_native_surface(api, null, 0, 0)
        shim.mdk_shim_destroy(api)
    }

    /** Attach a native window handle as the MDK render surface. */
    fun updateNativeSurface(nativeHandle: Pointer?, width: Int, height: Int) =
        shim.mdk_shim_update_native_surface(api, nativeHandle, width, height)

    /** Notify MDK that the surface dimensions changed. */
    fun setSurfaceSize(width: Int, height: Int) =
        shim.mdk_shim_set_surface_size(api, width, height)

    fun seek(positionMs: Long) = shim.mdk_shim_seek(api, positionMs)
    fun position(): Long       = shim.mdk_shim_position(api)
}

// ─────────────────────────────────────────────────────────────────────────────
// VideoSurface – Desktop actual
//
// Embeds an AWT Canvas via SwingPanel.  JNA's Native.getComponentID() returns
// the platform-specific native window handle:
//   Windows : HWND
//   macOS   : NSView*
//   Linux   : X11 Window (XID)
//
// MDK uses this handle to create its own rendering context (D3D11 on Windows,
// Metal on macOS, GL/Vulkan on Linux).
// ─────────────────────────────────────────────────────────────────────────────

@Composable
actual fun VideoSurface(player: MdkPlayer, modifier: Modifier) {
    SwingPanel(
        modifier = modifier,
        factory = {
            MdkCanvas(player)
        },
    )
}

private class MdkCanvas(private val player: MdkPlayer) : Canvas() {

    // addNotify is called once the component has a native peer (window handle)
    override fun addNotify() {
        super.addNotify()
        attachSurface()
    }

    override fun removeNotify() {
        player.updateNativeSurface(null, 0, 0)
        super.removeNotify()
    }

    override fun setBounds(x: Int, y: Int, w: Int, h: Int) {
        super.setBounds(x, y, w, h)
        if (isDisplayable && w > 0 && h > 0) {
            player.setSurfaceSize(w, h)
        }
    }

    private fun attachSurface() {
        val handle: Long = Native.getComponentID(this)
        if (handle != 0L && width > 0 && height > 0) {
            player.updateNativeSurface(Pointer(handle), width, height)
        }
    }
}
