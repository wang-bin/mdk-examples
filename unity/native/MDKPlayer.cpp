/*
 * Copyright (c) 2024 WangBin <wbsecg1 at gmail.com>
 * MDK SDK Unity Plugin — Native Implementation
 * Wraps mdk::Player with a plain C API for use via P/Invoke from Unity/C#.
 * Supports: Windows (D3D11/OpenGL), macOS (Metal/OpenGL), Linux (OpenGL),
 *           iOS (Metal), Android (OpenGL ES).
 */
#ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
#endif
#define MDK_UNITY_BUILD
#include "MDKPlayer.h"
#include "mdk/Player.h"
#include "mdk/RenderAPI.h"
#include "mdk/global.h"
#include <cstring>
#include <mutex>
#include <string>

using namespace MDK_NS;

/* -----------------------------------------------------------------------
 * Internal state
 * -------------------------------------------------------------------- */
struct MDKPlayerContext {
    Player player;

    // Render API storage (only one active at a time)
    GLRenderAPI    gl{};
    D3D11RenderAPI d3d11{};
#if defined(__APPLE__)
    MetalRenderAPI metal{};
#endif

    // User callbacks + userdata
    MDK_StateChangedCallback       onStateChanged       = nullptr;
    void*                          onStateChangedUD     = nullptr;
    MDK_RenderCallback             onRender             = nullptr;
    void*                          onRenderUD           = nullptr;
    MDK_MediaStatusChangedCallback onMediaStatus        = nullptr;
    void*                          onMediaStatusUD      = nullptr;
};

/* -----------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------- */
static inline MDKPlayerContext* ctx(MDKPlayerHandle h)
{
    return reinterpret_cast<MDKPlayerContext*>(h);
}

/* -----------------------------------------------------------------------
 * Lifecycle
 * -------------------------------------------------------------------- */
MDK_UNITY_API MDKPlayerHandle MDKPlayer_create()
{
    return reinterpret_cast<MDKPlayerHandle>(new MDKPlayerContext());
}

MDK_UNITY_API void MDKPlayer_destroy(MDKPlayerHandle player)
{
    delete ctx(player);
}

/* -----------------------------------------------------------------------
 * Media / Playback
 * -------------------------------------------------------------------- */
MDK_UNITY_API void MDKPlayer_setMedia(MDKPlayerHandle player, const char* url)
{
    ctx(player)->player.setMedia(url ? url : "");
}

MDK_UNITY_API const char* MDKPlayer_getMedia(MDKPlayerHandle player)
{
    return ctx(player)->player.url();
}

MDK_UNITY_API void MDKPlayer_setState(MDKPlayerHandle player, MDK_State state)
{
    ctx(player)->player.set(static_cast<State>(state));
}

MDK_UNITY_API MDK_State MDKPlayer_getState(MDKPlayerHandle player)
{
    return static_cast<MDK_State>(ctx(player)->player.state());
}

MDK_UNITY_API void MDKPlayer_seek(MDKPlayerHandle player, int64_t position_ms)
{
    ctx(player)->player.seek(position_ms);
}

MDK_UNITY_API int64_t MDKPlayer_getPosition(MDKPlayerHandle player)
{
    return ctx(player)->player.position();
}

MDK_UNITY_API int64_t MDKPlayer_getDuration(MDKPlayerHandle player)
{
    const auto& info = ctx(player)->player.mediaInfo();
    return info.duration;
}

MDK_UNITY_API void MDKPlayer_setPlaybackRate(MDKPlayerHandle player, float rate)
{
    ctx(player)->player.setPlaybackRate(rate);
}

MDK_UNITY_API float MDKPlayer_getPlaybackRate(MDKPlayerHandle player)
{
    return ctx(player)->player.playbackRate();
}

MDK_UNITY_API void MDKPlayer_setVolume(MDKPlayerHandle player, float volume)
{
    ctx(player)->player.setVolume(volume);
}

MDK_UNITY_API void MDKPlayer_setLoop(MDKPlayerHandle player, int count)
{
    ctx(player)->player.setLoop(count);
}

MDK_UNITY_API void MDKPlayer_setDecoders(MDKPlayerHandle player, const char* decoders)
{
    if (!decoders || decoders[0] == '\0') {
        ctx(player)->player.setDecoders(MediaType::Video, {"auto"});
        return;
    }
    // Split comma-separated list
    std::vector<std::string> list;
    const char* p = decoders;
    while (*p) {
        const char* comma = std::strchr(p, ',');
        if (!comma) {
            list.emplace_back(p);
            break;
        }
        list.emplace_back(p, comma - p);
        p = comma + 1;
    }
    // Build string_views after the vector is fully populated to avoid
    // dangling references into a container that may have been reallocated.
    std::vector<std::string_view> views;
    views.reserve(list.size());
    for (const auto& s : list)
        views.emplace_back(s);
    ctx(player)->player.setDecoders(MediaType::Video, views);
}

MDK_UNITY_API void MDKPlayer_prepare(MDKPlayerHandle player, int64_t position_ms)
{
    ctx(player)->player.prepare(position_ms);
}

MDK_UNITY_API int MDKPlayer_getMediaStatus(MDKPlayerHandle player)
{
    return static_cast<int>(ctx(player)->player.mediaStatus());
}

/* -----------------------------------------------------------------------
 * Video size
 * -------------------------------------------------------------------- */
MDK_UNITY_API bool MDKPlayer_getVideoSize(MDKPlayerHandle player, int* width, int* height)
{
    const auto& info = ctx(player)->player.mediaInfo();
    if (info.video.empty())
        return false;
    *width  = info.video[0].codec.width;
    *height = info.video[0].codec.height;
    return (*width > 0 && *height > 0);
}

/* -----------------------------------------------------------------------
 * Render surface
 * -------------------------------------------------------------------- */
MDK_UNITY_API void MDKPlayer_setVideoSurfaceSize(MDKPlayerHandle player, int width, int height)
{
    ctx(player)->player.setVideoSurfaceSize(width, height);
}

MDK_UNITY_API double MDKPlayer_renderVideo(MDKPlayerHandle player)
{
    return ctx(player)->player.renderVideo();
}

/* -----------------------------------------------------------------------
 * OpenGL render target
 * fbo_id = 0: create/manage FBO internally bound to texture_id.
 * -------------------------------------------------------------------- */
MDK_UNITY_API void MDKPlayer_setOpenGLRenderTarget(MDKPlayerHandle player,
                                                    intptr_t texture_id,
                                                    intptr_t fbo_id,
                                                    int width, int height)
{
    auto* c = ctx(player);
    c->gl = {};
    c->gl.fbo     = static_cast<unsigned>(fbo_id);
    c->gl.texture = static_cast<unsigned>(texture_id);
    c->player.setRenderAPI(&c->gl);
    c->player.setVideoSurfaceSize(width, height);
}

/* -----------------------------------------------------------------------
 * D3D11 render target (Windows)
 * -------------------------------------------------------------------- */
MDK_UNITY_API void MDKPlayer_setD3D11RenderTarget(MDKPlayerHandle player,
                                                   void* texture,
                                                   void* context)
{
#ifdef _WIN32
    auto* c = ctx(player);
    c->d3d11 = {};
    c->d3d11.texture = static_cast<ID3D11Texture2D*>(texture);
    c->d3d11.context = static_cast<ID3D11DeviceContext*>(context);
    c->player.setRenderAPI(&c->d3d11);
#else
    (void)player; (void)texture; (void)context;
#endif
}

/* -----------------------------------------------------------------------
 * Metal render target (Apple platforms)
 * -------------------------------------------------------------------- */
MDK_UNITY_API void MDKPlayer_setMetalRenderTarget(MDKPlayerHandle player,
                                                   void* texture,
                                                   void* device,
                                                   void* cmdQueue)
{
#if defined(__APPLE__)
    auto* c = ctx(player);
    c->metal = {};
    c->metal.texture  = texture;
    c->metal.device   = device;
    c->metal.cmdQueue = cmdQueue;
    c->player.setRenderAPI(&c->metal);
#else
    (void)player; (void)texture; (void)device; (void)cmdQueue;
#endif
}

/* -----------------------------------------------------------------------
 * Callbacks
 * -------------------------------------------------------------------- */
MDK_UNITY_API void MDKPlayer_setStateChangedCallback(MDKPlayerHandle player,
                                                      MDK_StateChangedCallback cb,
                                                      void* userdata)
{
    auto* c = ctx(player);
    c->onStateChanged   = cb;
    c->onStateChangedUD = userdata;
    c->player.onStateChanged([c](State s) {
        if (c->onStateChanged)
            c->onStateChanged(static_cast<MDK_State>(s), c->onStateChangedUD);
    });
}

MDK_UNITY_API void MDKPlayer_setRenderCallback(MDKPlayerHandle player,
                                                MDK_RenderCallback cb,
                                                void* userdata)
{
    auto* c = ctx(player);
    c->onRender   = cb;
    c->onRenderUD = userdata;
    c->player.setRenderCallback([c](void*) {
        if (c->onRender)
            c->onRender(c->onRenderUD);
    });
}

MDK_UNITY_API void MDKPlayer_setMediaStatusChangedCallback(MDKPlayerHandle player,
                                                            MDK_MediaStatusChangedCallback cb,
                                                            void* userdata)
{
    auto* c = ctx(player);
    c->onMediaStatus   = cb;
    c->onMediaStatusUD = userdata;
    c->player.onMediaStatusChanged([c](MediaStatus s) {
        if (c->onMediaStatus)
            c->onMediaStatus(static_cast<MDK_MediaStatus>(s), c->onMediaStatusUD);
        return true;
    });
}

/* -----------------------------------------------------------------------
 * Global
 * -------------------------------------------------------------------- */
MDK_UNITY_API void MDK_setLogLevel(int level)
{
    SetGlobalOption("log", level);
}
