/*
 * mdk_shim.c – Thin C wrapper around the MDK C API for use with JNA on desktop.
 *
 * The MDK C API (mdk/c/Player.h) exposes the player through a struct whose
 * fields are function pointers.  Calling those function pointers directly from
 * JNA requires complex Structure / Callback mappings.  This shim provides
 * plain exported C functions that JNA can call without any knowledge of the
 * internal struct layout.
 *
 * Build (Linux / macOS / Windows):
 *
 *   mkdir build && cd build
 *   cmake .. -DMDK_SDK_DIR=/path/to/mdk-sdk
 *   cmake --build .
 *
 * The resulting library (libmdk_shim.so / libmdk_shim.dylib / mdk_shim.dll)
 * must be placed on the java.library.path at runtime (e.g. next to the JAR,
 * or passed via -Djava.library.path=...).
 *
 * Download MDK SDK: https://sourceforge.net/projects/mdk-sdk/files/nightly/
 */

#include "mdk/c/Player.h"
#include <string.h>
#include <stdint.h>

/* Convenience macro: dereference a function pointer in the player struct */
#define CALL(api, fn, ...) \
    do { if ((api) && (api)->fn) (api)->fn((api), ##__VA_ARGS__); } while(0)

#define CALL_RET(api, fn, default_val, ...) \
    ((api) && (api)->fn ? (api)->fn((api), ##__VA_ARGS__) : (default_val))

#if defined(_WIN32)
#  define MDK_SHIM_EXPORT __declspec(dllexport)
#else
#  define MDK_SHIM_EXPORT __attribute__((visibility("default")))
#endif

/* ── Player lifetime ─────────────────────────────────────────────────────── */

MDK_SHIM_EXPORT void* mdk_shim_create(void) {
    return (void*)MDK_createPlayer();
}

MDK_SHIM_EXPORT void mdk_shim_destroy(void* api) {
    mdkPlayerAPI* p = (mdkPlayerAPI*)api;
    MDK_destroyPlayer(&p);
}

/* ── Playback control ────────────────────────────────────────────────────── */

MDK_SHIM_EXPORT void mdk_shim_set_media(void* api, const char* url) {
    CALL((mdkPlayerAPI*)api, setMedia, url, 0);
}

MDK_SHIM_EXPORT void mdk_shim_play(void* api) {
    CALL((mdkPlayerAPI*)api, set, MDKState_Playing);
}

MDK_SHIM_EXPORT void mdk_shim_pause(void* api) {
    CALL((mdkPlayerAPI*)api, set, MDKState_Paused);
}

MDK_SHIM_EXPORT void mdk_shim_stop(void* api) {
    CALL((mdkPlayerAPI*)api, set, MDKState_Stopped);
}

MDK_SHIM_EXPORT void mdk_shim_set_volume(void* api, float vol) {
    CALL((mdkPlayerAPI*)api, setVolume, vol);
}

MDK_SHIM_EXPORT void mdk_shim_set_playback_rate(void* api, float rate) {
    CALL((mdkPlayerAPI*)api, setPlaybackRate, rate);
}

/* ── Surface / rendering ─────────────────────────────────────────────────── */

MDK_SHIM_EXPORT void mdk_shim_update_native_surface(void* api, void* surface,
                                                     int width, int height) {
    CALL((mdkPlayerAPI*)api, updateNativeSurface, surface, width, height, 0);
}

MDK_SHIM_EXPORT void mdk_shim_set_surface_size(void* api, int width, int height) {
    CALL((mdkPlayerAPI*)api, setVideoSurfaceSize, width, height, NULL);
}

MDK_SHIM_EXPORT int mdk_shim_render_video(void* api) {
    return CALL_RET((mdkPlayerAPI*)api, renderVideo, 0, NULL) ? 1 : 0;
}

/* ── Seek / position ─────────────────────────────────────────────────────── */

MDK_SHIM_EXPORT void mdk_shim_seek(void* api, int64_t position_ms) {
    /* Pass 0 for flags – equivalent to MDKSeekFlag_Default (seek to keyframe) */
    CALL((mdkPlayerAPI*)api, seek, position_ms, 0, NULL, NULL);
}

MDK_SHIM_EXPORT int64_t mdk_shim_position(void* api) {
    return CALL_RET((mdkPlayerAPI*)api, position, (int64_t)0);
}

/* ── Global helpers ──────────────────────────────────────────────────────── */

MDK_SHIM_EXPORT void mdk_shim_set_global_option(const char* key, const char* value) {
    MDK_setGlobalOptionString(key, value);
}
