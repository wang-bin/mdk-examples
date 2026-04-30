/*
 * Copyright (c) 2024 WangBin <wbsecg1 at gmail.com>
 * MDK SDK Unity Plugin
 * C API for use via P/Invoke from Unity/C#
 * Supports: Windows, macOS, Linux, iOS, Android
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#if defined(_WIN32)
# ifdef MDK_UNITY_BUILD
#  define MDK_UNITY_API __declspec(dllexport)
# else
#  define MDK_UNITY_API __declspec(dllimport)
# endif
#else
# define MDK_UNITY_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* MDKPlayerHandle;

/* Playback state, matches mdk::State */
typedef enum {
    MDK_State_Stopped = 0,
    MDK_State_Playing = 1,
    MDK_State_Paused  = 2,
} MDK_State;

/* Media status flags, matches mdk::MediaStatus */
typedef enum {
    MDK_MediaStatus_NoMedia    = 0,
    MDK_MediaStatus_Unloaded   = 1,
    MDK_MediaStatus_Loading    = 2,
    MDK_MediaStatus_Loaded     = 4,
    MDK_MediaStatus_Prepared   = 8,
    MDK_MediaStatus_Stalled    = 16,
    MDK_MediaStatus_Buffering  = 32,
    MDK_MediaStatus_Buffered   = 64,
    MDK_MediaStatus_End        = 128,
    MDK_MediaStatus_Seeking    = 256,
    MDK_MediaStatus_Invalid    = (1<<31),
} MDK_MediaStatus;

/* Callback types */
typedef void (*MDK_StateChangedCallback)(MDK_State state, void* userdata);
typedef void (*MDK_PositionChangedCallback)(int64_t position_ms, void* userdata);
typedef void (*MDK_RenderCallback)(void* userdata);
typedef void (*MDK_MediaStatusChangedCallback)(MDK_MediaStatus status, void* userdata);

/*
 * Create a new player instance.
 * Returns an opaque handle; must be freed with MDKPlayer_destroy().
 */
MDK_UNITY_API MDKPlayerHandle MDKPlayer_create();

/* Destroy a player and free all resources. */
MDK_UNITY_API void MDKPlayer_destroy(MDKPlayerHandle player);

/*
 * Set the media URL to play.
 * url can be a local path or a network URL (http/https/rtsp/rtmp/hls/dash...).
 */
MDK_UNITY_API void MDKPlayer_setMedia(MDKPlayerHandle player, const char* url);

/* Get the current media URL. */
MDK_UNITY_API const char* MDKPlayer_getMedia(MDKPlayerHandle player);

/* Set playback state: MDK_State_Playing, MDK_State_Paused, or MDK_State_Stopped. */
MDK_UNITY_API void MDKPlayer_setState(MDKPlayerHandle player, MDK_State state);

/* Get current playback state. */
MDK_UNITY_API MDK_State MDKPlayer_getState(MDKPlayerHandle player);

/* Seek to position in milliseconds. */
MDK_UNITY_API void MDKPlayer_seek(MDKPlayerHandle player, int64_t position_ms);

/* Get current playback position in milliseconds. */
MDK_UNITY_API int64_t MDKPlayer_getPosition(MDKPlayerHandle player);

/* Get total media duration in milliseconds (0 if unknown). */
MDK_UNITY_API int64_t MDKPlayer_getDuration(MDKPlayerHandle player);

/* Set playback speed multiplier (e.g. 1.0 = normal, 2.0 = 2x). */
MDK_UNITY_API void MDKPlayer_setPlaybackRate(MDKPlayerHandle player, float rate);

/* Get current playback speed. */
MDK_UNITY_API float MDKPlayer_getPlaybackRate(MDKPlayerHandle player);

/* Set master audio volume (0.0 = mute, 1.0 = full). */
MDK_UNITY_API void MDKPlayer_setVolume(MDKPlayerHandle player, float volume);

/* Enable or disable looping. */
MDK_UNITY_API void MDKPlayer_setLoop(MDKPlayerHandle player, int count); /* -1 = infinite, 0 = no loop */

/*
 * Set the render target size (in pixels).
 * Call this when the render surface / texture size changes.
 * Pass -1x-1 to release GL renderer resources (call before destroying the GL context).
 */
MDK_UNITY_API void MDKPlayer_setVideoSurfaceSize(MDKPlayerHandle player, int width, int height);

/*
 * Render a video frame into the currently bound OpenGL FBO (or state set via setRenderAPI).
 * Must be called from the render thread with the correct GL context current.
 * Returns the current frame timestamp in ms, or <0 if no new frame was rendered.
 */
MDK_UNITY_API double MDKPlayer_renderVideo(MDKPlayerHandle player);

/*
 * Set a native OpenGL texture as the render target.
 * texture_id: OpenGL texture name (GLuint) cast to intptr_t.
 * fbo_id: OpenGL FBO name (GLuint) that has texture_id attached as color attachment 0.
 *         Pass 0 to let the plugin create/manage its own FBO internally.
 * This must be called from the render thread with the GL context current.
 */
MDK_UNITY_API void MDKPlayer_setOpenGLRenderTarget(MDKPlayerHandle player, intptr_t texture_id, intptr_t fbo_id, int width, int height);

/*
 * Set a native D3D11 texture as the render target (Windows only).
 * texture: pointer to an ID3D11Texture2D.
 * context: pointer to an ID3D11DeviceContext, or NULL to use the texture's device context.
 */
MDK_UNITY_API void MDKPlayer_setD3D11RenderTarget(MDKPlayerHandle player, void* texture, void* context);

/*
 * Set a native Metal texture as the render target (Apple platforms only).
 * texture: pointer to a MTLTexture (id<MTLTexture> bridged to void*).
 * device:  pointer to a MTLDevice  (id<MTLDevice>  bridged to void*).
 * cmdQueue: pointer to a MTLCommandQueue (id<MTLCommandQueue> bridged to void*), or NULL.
 */
MDK_UNITY_API void MDKPlayer_setMetalRenderTarget(MDKPlayerHandle player, void* texture, void* device, void* cmdQueue);

/*
 * Register a callback invoked when the player state changes.
 * Callback is invoked from an internal thread; do not call MDK API from within it.
 */
MDK_UNITY_API void MDKPlayer_setStateChangedCallback(MDKPlayerHandle player, MDK_StateChangedCallback cb, void* userdata);

/*
 * Register a callback invoked each time a new video frame is ready to render.
 * Use this to know when to call MDKPlayer_renderVideo().
 * The callback is invoked from a decoding thread; post to the render thread if needed.
 */
MDK_UNITY_API void MDKPlayer_setRenderCallback(MDKPlayerHandle player, MDK_RenderCallback cb, void* userdata);

/*
 * Register a callback invoked when the media status changes.
 */
MDK_UNITY_API void MDKPlayer_setMediaStatusChangedCallback(MDKPlayerHandle player, MDK_MediaStatusChangedCallback cb, void* userdata);

/*
 * Get video dimensions. Returns false if unavailable (media not yet loaded).
 */
MDK_UNITY_API bool MDKPlayer_getVideoSize(MDKPlayerHandle player, int* width, int* height);

/*
 * Set preferred hardware video decoder names.
 * decoders: comma-separated list, e.g. "VideoToolbox,FFmpeg" or "D3D11,DXVA,FFmpeg"
 * Pass NULL or empty string to reset to defaults.
 */
MDK_UNITY_API void MDKPlayer_setDecoders(MDKPlayerHandle player, const char* decoders);

/*
 * Prepare playback at the given position (ms). Decoding starts immediately.
 * Call MDKPlayer_setState(MDK_State_Playing) afterwards to start rendering.
 * position_ms = 0 means start from the beginning.
 */
MDK_UNITY_API void MDKPlayer_prepare(MDKPlayerHandle player, int64_t position_ms);

/*
 * Get the current media status flags (bitfield of MDK_MediaStatus values).
 */
MDK_UNITY_API int MDKPlayer_getMediaStatus(MDKPlayerHandle player);

/*
 * Global MDK logging control.
 * level: 0=Off, 1=Error, 2=Warning, 3=Info, 4=Debug, 5=All
 */
MDK_UNITY_API void MDK_setLogLevel(int level);

#ifdef __cplusplus
} // extern "C"
#endif
