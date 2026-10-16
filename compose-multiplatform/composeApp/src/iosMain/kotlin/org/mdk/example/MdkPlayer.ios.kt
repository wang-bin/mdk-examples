package org.mdk.example

import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.interop.UIKitView
import kotlinx.cinterop.*
import mdk.cinterop.*
import platform.CoreGraphics.CGRectZero
import platform.QuartzCore.CAMetalLayer
import platform.UIKit.UIView

// ─────────────────────────────────────────────────────────────────────────────
// MdkPlayer – iOS actual
//
// Uses the MDK C API exposed via Kotlin/Native C interop (see mdk.def).
// MDK_createPlayer / MDK_destroyPlayer are the factory functions; all other
// operations go through function pointers inside the returned mdkPlayerAPI
// struct, matching the pattern described in mdk/c/Player.h.
// ─────────────────────────────────────────────────────────────────────────────

@OptIn(ExperimentalForeignApi::class)
actual class MdkPlayer actual constructor() {

    // mdkPlayerAPI* returned by MDK_createPlayer()
    private val api: CPointer<mdkPlayerAPI> =
        MDK_createPlayer() ?: error("MDK_createPlayer() returned null")

    // Holds the current render-callback stable ref so it can be disposed when
    // a new callback is registered or when the player is released.
    private var renderCallbackRef: StableRef<() -> Unit>? = null

    actual fun setMedia(url: String) {
        memScoped {
            api.pointed.setMedia?.invoke(api, url.cstr.ptr, 0)
        }
    }

    actual fun play() {
        api.pointed.set?.invoke(api, MDKState_Playing)
    }

    actual fun pause() {
        api.pointed.set?.invoke(api, MDKState_Paused)
    }

    actual fun stop() {
        api.pointed.set?.invoke(api, MDKState_Stopped)
    }

    actual fun release() {
        // Clear render callback and dispose its stable ref before destroying
        api.pointed.setRenderCallback?.invoke(api, null, null, null)
        renderCallbackRef?.dispose()
        renderCallbackRef = null

        memScoped {
            // MDK_destroyPlayer takes mdkPlayerAPI** and nulls the pointer
            val apiRef = allocPointerTo<mdkPlayerAPI>()
            apiRef.value = api
            MDK_destroyPlayer(apiRef.ptr)
        }
    }

    /**
     * Attach [view] as the native rendering surface.
     *
     * The ObjC bridge helper [mdkUIViewPtr] (defined in mdk.def) performs an
     * ARC __bridge cast to obtain the raw C pointer without affecting the
     * reference count.
     */
    fun updateNativeSurface(view: UIView, width: Int, height: Int) {
        val surfacePtr: COpaquePointer? = mdkUIViewPtr(view)
        api.pointed.updateNativeSurface?.invoke(api, surfacePtr, width, height, 0)
    }

    /** Notify MDK that the surface dimensions changed. */
    fun setVideoSurfaceSize(width: Int, height: Int) {
        api.pointed.setVideoSurfaceSize?.invoke(api, width, height, null)
    }

    /**
     * Register a callback that MDK calls whenever a new video frame is ready
     * to be rendered.  The [callback] should schedule a redraw on the main
     * thread (e.g. call setNeedsDisplay on the view).
     *
     * Any previously registered callback's [StableRef] is disposed here to
     * prevent memory leaks.
     */
    fun setRenderCallback(callback: () -> Unit) {
        // Dispose the previous stable ref before creating a new one
        renderCallbackRef?.dispose()
        val stableRef = StableRef.create(callback)
        renderCallbackRef = stableRef
        api.pointed.setRenderCallback?.invoke(
            api,
            staticCFunction { opaque: COpaquePointer? ->
                opaque?.asStableRef<() -> Unit>()?.get()?.invoke()
            },
            stableRef.asCPointer(),
            null,
        )
    }

    /** Render one video frame to the attached surface; returns true if a frame was rendered. */
    fun renderVideo(): Boolean =
        api.pointed.renderVideo?.invoke(api, null) ?: false
}

// ─────────────────────────────────────────────────────────────────────────────
// VideoSurface – iOS actual
//
// Embeds a UIView via UIKitView.  On every layout pass the view passes its
// CAMetalLayer (or itself) to MDK as the render target.
// ─────────────────────────────────────────────────────────────────────────────

@Composable
actual fun VideoSurface(player: MdkPlayer, modifier: Modifier) {
    UIKitView(
        factory = {
            MdkUIView(player = player)
        },
        modifier = modifier,
        update = { view ->
            (view as? MdkUIView)?.syncSize()
        },
    )
}

/** A UIView whose CAMetalLayer is used as the MDK render target. */
@OptIn(ExperimentalForeignApi::class)
private class MdkUIView(private val player: MdkPlayer) :
    UIView(frame = CGRectZero.readValue()) {

    init {
        // Use a CAMetalLayer as the backing layer; MDK auto-detects it.
        layer.addSublayer(CAMetalLayer())
    }

    override fun layoutSubviews() {
        super.layoutSubviews()
        syncSize()
    }

    fun syncSize() {
        val w = bounds.useContents { size.width.toInt() }.coerceAtLeast(1)
        val h = bounds.useContents { size.height.toInt() }.coerceAtLeast(1)
        // Pass the UIView itself – MDK will discover the CAMetalLayer from it.
        player.updateNativeSurface(this, w, h)
        player.setVideoSurfaceSize(w, h)
    }
}
