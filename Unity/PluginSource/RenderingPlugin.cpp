/*
 * Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
#include "MDKUnityPlugin.h"

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include <map>
#include <utility>

using namespace mdk;

namespace {

constexpr int kRenderEvent = 1;
constexpr int kReleaseEvent = 2;

std::mutex g_registryMutex;
std::map<uint64_t, std::shared_ptr<MDKUnityPlayer>> g_players;
std::atomic<uint64_t> g_nextHandle{1};
IUnityInterfaces* g_interfaces = nullptr;
IUnityGraphics* g_graphics = nullptr;
std::atomic<int> g_renderer{kUnityGfxRendererNull};

UnityGfxRenderer CurrentRenderer()
{
    // A managed P/Invoke can create the player after Unity has initialized
    // the graphics device but before the device callback reaches this DLL.
    // Refreshing the renderer at the event boundary avoids keeping the null
    // value captured during that small ordering window.
    if (g_graphics) {
        const auto renderer = g_graphics->GetRenderer();
        if (renderer != kUnityGfxRendererNull)
            g_renderer.store(static_cast<int>(renderer), std::memory_order_release);
    }
    return static_cast<UnityGfxRenderer>(g_renderer.load(std::memory_order_acquire));
}

std::shared_ptr<MDKUnityPlayer> FindPlayer(uint64_t handle)
{
    const std::lock_guard lock(g_registryMutex);
    if (auto it = g_players.find(handle); it != g_players.end())
        return it->second;
    return {};
}

void OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    if (g_graphics)
        g_renderer.store(static_cast<int>(g_graphics->GetRenderer()), std::memory_order_release);

    if (eventType == kUnityGfxDeviceEventShutdown || eventType == kUnityGfxDeviceEventBeforeReset) {
        const MDKUnityOpenGLStateGuard glState(CurrentRenderer());
        const std::lock_guard lock(g_registryMutex);
        for (auto& [_, player] : g_players) {
            const std::lock_guard renderLock(player->renderMutex);
            const std::lock_guard apiLock(player->apiMutex);
            player->player.setVideoSurfaceSize(-1, -1, player.get());
            if (player->backendReady)
                MDKUnityBackendRelease(*player, static_cast<UnityGfxRenderer>(g_renderer.load(std::memory_order_acquire)));
            player->backendReady = false;
            player->renderApi.reset();
            player->backendState = nullptr;
        }
    }
    // Release render-thread resources while backend interfaces are still valid;
    // only then invalidate the device-specific caches for the next reset.
    MDKUnityBackendDeviceEvent(eventType, g_interfaces,
                               static_cast<UnityGfxRenderer>(g_renderer.load(std::memory_order_acquire)));
}

void UNITY_INTERFACE_API OnRenderEventAndData(int eventId, void* data)
{
    const auto handle = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(data));
    auto player = FindPlayer(handle);
    if (!player)
        return;

    if (CurrentRenderer() == kUnityGfxRendererVulkan &&
        MDKUnityVulkanDispatchEvent(eventId, player, OnRenderEventAndData))
        return;

    const MDKUnityOpenGLStateGuard glState(CurrentRenderer());

    if (eventId == kReleaseEvent || player->destroyRequested()) {
        {
            const std::lock_guard lock(player->renderMutex);
            player->finalizeOnRenderThread(static_cast<UnityGfxRenderer>(g_renderer.load(std::memory_order_acquire)));
        }
        const std::lock_guard lock(g_registryMutex);
        g_players.erase(handle);
        return;
    }
    if (eventId != kRenderEvent)
        return;

    player->render(CurrentRenderer(), MDKUnityVulkanEventTarget());
}

} // namespace

IUnityInterfaces* MDKUnityInterfaces() { return g_interfaces; }
IUnityGraphics* MDKUnityGraphics() { return g_graphics; }

MDKUnityPlayer::MDKUnityPlayer(uint64_t handle)
    : handle_(handle)
{
    player.onStateChanged([this](PlaybackState state) {
        state_.store(static_cast<int32_t>(state), std::memory_order_release);
        return;
    });
    player.onMediaStatus([this](MediaStatus, MediaStatus status) {
        mediaStatus_.store(static_cast<int32_t>(status), std::memory_order_release);
        if (test_flag(status, MediaStatus::Invalid)) {
            error_.store(1, std::memory_order_release);
            const std::lock_guard errorLock(errorMutex);
            renderErrorMessage = "media is invalid or unsupported";
        }
        return true;
    });
}

MDKUnityPlayer::~MDKUnityPlayer() = default;

void MDKUnityPlayer::setMedia(const char* url)
{
    const std::lock_guard lock(apiMutex);
    player.set(State::Stopped);
// Vulkan foreign contexts require CPU-copy output from hardware decoders.
    if (CurrentRenderer() == kUnityGfxRendererVulkan) {
#if defined(_WIN32)
        player.setDecoders(MediaType::Video, {"D3D11:copy=1", "FFmpeg"});
#elif defined(__ANDROID__)
        player.setDecoders(MediaType::Video, {"AMediaCodec:copy=1", "FFmpeg"});
#elif defined(__linux__)
        player.setDecoders(MediaType::Video, {"VAAPI:copy=1", "FFmpeg"});
#endif
    } else if (const auto* decoder = std::getenv("MDK_UNITY_VIDEO_DECODER"); decoder && *decoder) {
// An explicit diagnostic decoder has no fallback, so hardware failures stay visible.
        player.setDecoders(MediaType::Video, {decoder});
    }
    player.setMedia(url ? url : "");
    error_.store(0, std::memory_order_release);
    {
        const std::lock_guard errorLock(errorMutex);
        renderErrorMessage.clear();
    }
    player.set(State::Playing);
}

void MDKUnityPlayer::setState(int state)
{
    const std::lock_guard lock(apiMutex);
    const auto value = std::clamp(state, 0, 2);
    player.set(static_cast<State>(value));
}

void MDKUnityPlayer::setLoop(int count)
{
    const std::lock_guard lock(apiMutex);
    player.setLoop(std::max(count, 0));
}

void MDKUnityPlayer::setTarget(const UnityRenderTarget& value)
{
    const std::lock_guard lock(targetMutex);
    target = value;
}

void MDKUnityPlayer::requestDestroy()
{
    destroyRequested_.store(true, std::memory_order_release);
}

void MDKUnityPlayer::fillInfo(MDKUnityPlayerInfo* out) const
{
    if (!out)
        return;
    *out = {};
    out->pixelAspect = 1.0f;
    out->state = state_.load(std::memory_order_acquire);
    out->mediaStatus = mediaStatus_.load(std::memory_order_acquire);
    out->renderer = static_cast<int32_t>(CurrentRenderer());
    out->error = error_.load(std::memory_order_acquire);
    {
        const std::lock_guard targetLock(targetMutex);
        out->targetVersion = target.version;
        out->appliedVersion = appliedVersion;
    }
    const std::lock_guard lock(apiMutex);
    out->state = static_cast<int32_t>(player.state());
    out->mediaStatus = static_cast<int32_t>(player.mediaStatus());
    out->position = player.position();
    const auto& info = player.mediaInfo();
    out->duration = info.duration;
    if (!info.video.empty()) {
        const auto& video = info.video.front();
        out->width = video.codec.width;
        out->height = video.codec.height;
        out->pixelAspect = video.codec.par > 0 ? video.codec.par : 1.0f;
        out->rotation = video.rotation;
    }
}

std::string MDKUnityPlayer::error() const
{
    const std::lock_guard lock(errorMutex);
    return renderErrorMessage;
}

bool MDKUnityPlayer::renderFailure(const char* message)
{
    error_.store(1, std::memory_order_release);
    const std::lock_guard lock(errorMutex);
    renderErrorMessage = message;
    return false;
}

bool MDKUnityPlayer::render(UnityGfxRenderer renderer, const UnityRenderTarget* snapshot)
{
    const std::lock_guard lock(renderMutex);
    if (destroyRequested())
        return false;
    UnityRenderTarget current;
    if (snapshot) {
        current = *snapshot;
    } else {
        const std::lock_guard targetLock(targetMutex);
        current = target;
    }
    if (!current.nativeTexture || current.width <= 0 || current.height <= 0)
        return false;
    if (current.outputMode == 1 && renderer != kUnityGfxRendererD3D11 && renderer != kUnityGfxRendererD3D12)
        return renderFailure("HDRLinear requires D3D11 or D3D12");
// All backend work in this event uses the same version and output-mode snapshot.
    renderingTarget = current;

    if (!backendReady || !renderApi || appliedVersion != current.version) {
        if (backendReady) {
            const std::lock_guard apiLock(apiMutex);
            player.setVideoSurfaceSize(-1, -1, this);
            MDKUnityBackendRelease(*this, renderer);
        }
        renderApi.reset();
        {
            const std::lock_guard errorLock(errorMutex);
            renderErrorMessage.clear();
        }
        backendReady = MDKUnityBackendInitialize(*this, renderer);
        if (!backendReady) {
            error_.store(1, std::memory_order_release);
            const std::lock_guard errorLock(errorMutex);
            if (renderErrorMessage.empty())
                renderErrorMessage = "Unity render backend is unavailable";
            return false;
        }
        {
            const std::lock_guard errorLock(errorMutex);
            if (!test_flag(static_cast<MediaStatus>(mediaStatus_.load(std::memory_order_acquire)), MediaStatus::Invalid)) {
                renderErrorMessage.clear();
                error_.store(0, std::memory_order_release);
            }
        }
        {
            const std::lock_guard apiLock(apiMutex);
            player.set(current.outputMode == 1 ? ColorSpaceSCRGB : ColorSpaceBT709, this);
        }
        {
            const std::lock_guard targetLock(targetMutex);
            appliedVersion = current.version;
        }
    }
    const std::lock_guard apiLock(apiMutex);
    player.setVideoSurfaceSize(current.width, current.height, this);
    return MDKUnityBackendRender(*this, renderer);
}

void MDKUnityPlayer::finalizeOnRenderThread(UnityGfxRenderer renderer)
{
    const std::lock_guard apiLock(apiMutex);
    player.set(State::Stopped);
    player.setVideoSurfaceSize(-1, -1, this);
    if (backendReady)
        MDKUnityBackendRelease(*this, renderer);
    backendReady = false;
    renderApi.reset();
}

extern "C" uint64_t UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
MDKUnityPlayerCreate()
{
    const auto handle = g_nextHandle.fetch_add(1, std::memory_order_relaxed);
    auto player = std::make_shared<MDKUnityPlayer>(handle);
    const std::lock_guard lock(g_registryMutex);
    g_players.emplace(handle, std::move(player));
    return handle;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
MDKUnityPlayerDestroy(uint64_t handle)
{
    if (auto player = FindPlayer(handle))
        player->requestDestroy();
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
MDKUnityPlayerSetMedia(uint64_t handle, const char* url)
{
    if (auto player = FindPlayer(handle))
        player->setMedia(url);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
MDKUnityPlayerSetState(uint64_t handle, int state)
{
    if (auto player = FindPlayer(handle))
        player->setState(state);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
MDKUnityPlayerSetLoop(uint64_t handle, int count)
{
    if (auto player = FindPlayer(handle))
        player->setLoop(count);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
MDKUnityPlayerSetRenderTarget(uint64_t handle, void* nativeTexture, int width, int height, uint64_t version)
{
    if (auto player = FindPlayer(handle))
        player->setTarget({nativeTexture, width, height, version});
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
MDKUnityPlayerSetRenderTargetV2(uint64_t handle, const MDKUnityRenderTargetV2* target)
{
    auto player = FindPlayer(handle);
    if (!player)
        return 0;
    if (!target || target->structSize < offsetof(MDKUnityRenderTargetV2, version) + sizeof(uint64_t) ||
        target->outputMode < 0 || target->outputMode > 1 || !target->nativeTexture ||
        target->width <= 0 || target->height <= 0) {
        player->renderFailure("Invalid RenderTargetV2 descriptor");
        return 0;
    }
    player->setTarget({target->nativeTexture, target->width, target->height, target->version, target->outputMode});
    return 1;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
MDKUnityPlayerGetInfo(uint64_t handle, MDKUnityPlayerInfo* info)
{
    if (auto player = FindPlayer(handle))
        player->fillInfo(info);
    else if (info)
        *info = {};
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
MDKUnityPlayerCopyError(uint64_t handle, char* buffer, int capacity)
{
    if (!buffer || capacity <= 0)
        return 0;
    const auto player = FindPlayer(handle);
    const auto message = player ? player->error() : std::string{};
    const auto n = std::min<int>(static_cast<int>(message.size()), capacity - 1);
    std::memcpy(buffer, message.data(), static_cast<size_t>(n));
    buffer[n] = 0;
    return n;
}

extern "C" UnityRenderingEventAndData UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
MDKUnityGetRenderEventFunc()
{
    return OnRenderEventAndData;
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
MDKUnityGetRenderer()
{
    return static_cast<int>(CurrentRenderer());
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces* interfaces)
{
    g_interfaces = interfaces;
    g_graphics = interfaces ? interfaces->Get<IUnityGraphics>() : nullptr;
    if (g_graphics) {
        g_graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
        g_renderer.store(static_cast<int>(g_graphics->GetRenderer()), std::memory_order_release);
    }
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginUnload()
{
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventShutdown);
    if (g_graphics)
        g_graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
    const std::lock_guard lock(g_registryMutex);
    g_players.clear();
    g_graphics = nullptr;
    g_interfaces = nullptr;
    g_renderer.store(kUnityGfxRendererNull, std::memory_order_release);
}
