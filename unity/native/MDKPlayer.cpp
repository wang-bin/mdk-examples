/*
 * Copyright (c) 2024 WangBin <wbsecg1 at gmail.com>
 * MDK SDK Unity Plugin — Native Implementation
 * Wraps mdk::Player with a plain C API for use via P/Invoke from Unity/C#.
 * Supports: Windows (D3D11/D3D12/OpenGL), macOS (Metal/OpenGL), Linux (OpenGL/Vulkan),
 *           iOS (Metal), Android (OpenGL ES), and any platform with Vulkan support.
 */
#ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
#endif
#define MDK_UNITY_BUILD
#include "MDKPlayer.h"
#include "mdk/Player.h"
#include "mdk/RenderAPI.h"
#include "mdk/global.h"
#ifdef _WIN32
# include <d3d11.h>
# include <d3d12.h>
#endif
#if __has_include(<vulkan/vulkan_core.h>)
# include <vulkan/vulkan_core.h>
#endif
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
#ifdef _WIN32
    D3D12RenderAPI d3d12{};
#endif
#if defined(__APPLE__)
    MetalRenderAPI metal{};
#endif
#if __has_include(<vulkan/vulkan_core.h>)
    VulkanRenderAPI vk{};
#endif

    // User callbacks + userdata
    MDK_StateChangedCallback       onStateChanged       = nullptr;
    void*                          onStateChangedUD     = nullptr;
    MDK_RenderCallback             onRender             = nullptr;
    void*                          onRenderUD           = nullptr;
    MDK_MediaStatusChangedCallback onMediaStatus        = nullptr;
    void*                          onMediaStatusUD      = nullptr;

    // D3D12 render callbacks (stored here; RenderAPI.opaque = this context)
    MDK_D3D12GetCommandListCallback d3d12GetCmdList = nullptr;
    void*                           d3d12Opaque     = nullptr;

    // Vulkan render callbacks (stored here; RenderAPI.opaque = this context)
    MDK_VkRenderTargetInfoCallback vkGetRTInfo  = nullptr;
    MDK_VkGetCommandBufferCallback vkGetCmdBuf  = nullptr;
    void*                          vkOpaque     = nullptr;
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
 * D3D12 render target (Windows)
 *
 * getCmdList is a user-supplied callback invoked by MDK each time it needs
 * a command list to record rendering commands.  In Unity, implement it as a
 * native plugin callback that returns the current frame command list from
 * IUnityGraphicsD3D12v5::GetFrameCommandList().
 * -------------------------------------------------------------------- */
MDK_UNITY_API void MDKPlayer_setD3D12RenderTarget(MDKPlayerHandle player,
                                                   void* rt,
                                                   void* cmdQueue,
                                                   MDK_D3D12GetCommandListCallback getCmdList,
                                                   void* opaque)
{
#ifdef _WIN32
    auto* c = ctx(player);
    c->d3d12 = {};
    c->d3d12GetCmdList = getCmdList;
    c->d3d12Opaque     = opaque;
    c->d3d12.rt       = static_cast<ID3D12Resource*>(rt);
    c->d3d12.cmdQueue = static_cast<ID3D12CommandQueue*>(cmdQueue);
    if (getCmdList) {
        // Pass the context as opaque so the lambda can reach our stored callbacks.
        c->d3d12.opaque = c;
        c->d3d12.currentCommandList = [](const void* op) -> ID3D12GraphicsCommandList* {
            const auto* c = static_cast<const MDKPlayerContext*>(op);
            return static_cast<ID3D12GraphicsCommandList*>(
                c->d3d12GetCmdList(c->d3d12Opaque));
        };
    }
    c->player.setRenderAPI(&c->d3d12);
#else
    (void)player; (void)rt; (void)cmdQueue; (void)getCmdList; (void)opaque;
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
 * Vulkan render target (any platform with Vulkan support)
 *
 * getRTInfo: callback that provides the image dimensions, VkFormat (as int),
 *            and the expected VkImageLayout after rendering.
 * getCmdBuf: callback that returns the currently recording VkCommandBuffer
 *            (as void*) so MDK can record into it.
 * opaque:    forwarded to both callbacks.
 * -------------------------------------------------------------------- */
MDK_UNITY_API void MDKPlayer_setVulkanRenderTarget(MDKPlayerHandle player,
                                                    void* device,
                                                    void* phy_device,
                                                    void* rt,
                                                    MDK_VkRenderTargetInfoCallback getRTInfo,
                                                    MDK_VkGetCommandBufferCallback getCmdBuf,
                                                    void* opaque)
{
#if __has_include(<vulkan/vulkan_core.h>)
    auto* c = ctx(player);
    c->vk           = {};
    c->vkGetRTInfo  = getRTInfo;
    c->vkGetCmdBuf  = getCmdBuf;
    c->vkOpaque     = opaque;
    c->vk.device     = static_cast<VkDevice>(device);
    c->vk.phy_device = static_cast<VkPhysicalDevice>(phy_device);
    c->vk.rt         = static_cast<VkImage>(rt);
    // Use the context as the single opaque so both lambdas can reach our stored callbacks.
    c->vk.opaque = c;
    if (getRTInfo) {
        c->vk.renderTargetInfo = [](void* op, int* w, int* h,
                                    VkFormat* fmt, VkImageLayout* layout) -> int {
            auto* c = static_cast<MDKPlayerContext*>(op);
            int vkFmt    = VK_FORMAT_R8G8B8A8_UNORM;
            int vkLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            int r = c->vkGetRTInfo(c->vkOpaque, w, h, &vkFmt, &vkLayout);
            *fmt    = static_cast<VkFormat>(vkFmt);
            *layout = static_cast<VkImageLayout>(vkLayout);
            return r;
        };
    }
    if (getCmdBuf) {
        c->vk.currentCommandBuffer = [](void* op) -> VkCommandBuffer {
            auto* c = static_cast<MDKPlayerContext*>(op);
            return static_cast<VkCommandBuffer>(c->vkGetCmdBuf(c->vkOpaque));
        };
    }
    c->player.setRenderAPI(&c->vk);
#else
    (void)player; (void)device; (void)phy_device; (void)rt;
    (void)getRTInfo; (void)getCmdBuf; (void)opaque;
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
