/*
 * Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 *
 * Unity bridge for the MDK SDK. The public C functions in this file are the
 * only ABI surface consumed by the managed Unity package.
 */
#pragma once

#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX
#endif
#if defined(_WIN32)
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

#include "Unity/IUnityGraphics.h"
#include "Unity/IUnityInterface.h"
#if defined(_WIN32)
#include <d3d11.h>
#include <d3d12.h>
#endif
#if defined(_WIN32) || defined(__linux__) || defined(__ANDROID__)
#include <vulkan/vulkan.h>
#endif
#include <mdk/MediaInfo.h>
#include <mdk/Player.h>
#include <mdk/RenderAPI.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

struct MDKUnityPlayerInfo {
    int32_t state = 0;
    int32_t mediaStatus = 0;
    int32_t width = 0;
    int32_t height = 0;
    float pixelAspect = 1.0f;
    int32_t rotation = 0;
    int64_t position = 0;
    int64_t duration = 0;
    uint64_t targetVersion = 0;
    uint64_t appliedVersion = 0;
    int32_t renderer = kUnityGfxRendererNull;
    int32_t error = 0;
};

struct UnityRenderTarget {
    void* nativeTexture = nullptr;
    int width = 0;
    int height = 0;
    uint64_t version = 0;
    int32_t outputMode = 0;
};

// V2 is additive; the original target function and PlayerInfo ABI remain unchanged.
struct MDKUnityRenderTargetV2 {
    uint32_t structSize;
// Stable ABI values: 0 = SDR, 1 = HDRLinear (linear scRGB, 1.0 = 80 nit).
    int32_t outputMode;
    void* nativeTexture;
    int32_t width;
    int32_t height;
    uint64_t version;
};

class MDKUnityPlayer final {
public:
    explicit MDKUnityPlayer(uint64_t handle);
    ~MDKUnityPlayer();

    MDKUnityPlayer(const MDKUnityPlayer&) = delete;
    MDKUnityPlayer& operator=(const MDKUnityPlayer&) = delete;

    uint64_t handle() const { return handle_; }
    void setMedia(const char* url);
    void setState(int state);
    void setLoop(int count);
    void setTarget(const UnityRenderTarget& target);
    void requestDestroy();
    bool destroyRequested() const { return destroyRequested_.load(std::memory_order_acquire); }

    void fillInfo(MDKUnityPlayerInfo* out) const;
    std::string error() const;
    bool renderFailure(const char* message);

    // Called only from the Unity render thread.
    bool render(UnityGfxRenderer renderer, const UnityRenderTarget* snapshot = nullptr);
    void finalizeOnRenderThread(UnityGfxRenderer renderer);

    mutable std::mutex apiMutex;
    mutable std::mutex renderMutex;
    mutable std::mutex targetMutex;
    mutable std::mutex errorMutex;
    mdk::Player player;
    std::unique_ptr<mdk::RenderAPI> renderApi;
    UnityRenderTarget target;
    UnityRenderTarget renderingTarget;
    uint64_t appliedVersion = 0;
    std::string renderErrorMessage;
    bool backendReady = false;
    void* backendState = nullptr;

private:
    uint64_t handle_ = 0;
    std::atomic<bool> destroyRequested_{false};
    std::atomic<int32_t> state_{0};
    std::atomic<int32_t> mediaStatus_{0};
    std::atomic<int32_t> error_{0};
};

// Implemented by the platform backend translation unit.
bool MDKUnityBackendInitialize(MDKUnityPlayer& player, UnityGfxRenderer renderer);
bool MDKUnityBackendRender(MDKUnityPlayer& player, UnityGfxRenderer renderer);
void MDKUnityBackendRelease(MDKUnityPlayer& player, UnityGfxRenderer renderer);
void MDKUnityBackendDeviceEvent(UnityGfxDeviceEventType eventType, IUnityInterfaces* interfaces,
                                UnityGfxRenderer renderer);
const char* MDKUnityBackendName(UnityGfxRenderer renderer);

// Preserve Unity's GL state across rendering, target replacement and player destruction.
class MDKUnityOpenGLStateGuard final {
public:
    explicit MDKUnityOpenGLStateGuard(UnityGfxRenderer renderer);
    ~MDKUnityOpenGLStateGuard();
    MDKUnityOpenGLStateGuard(const MDKUnityOpenGLStateGuard&) = delete;
    MDKUnityOpenGLStateGuard& operator=(const MDKUnityOpenGLStateGuard&) = delete;
private:
    void* state_ = nullptr;
};

// Shared Unity interface state used by platform backends.
IUnityInterfaces* MDKUnityInterfaces();
IUnityGraphics* MDKUnityGraphics();

// Queue callbacks keep the player and the exact accessed texture version alive.
bool MDKUnityVulkanDispatchEvent(int eventId, const std::shared_ptr<MDKUnityPlayer>& player,
                                 UnityRenderingEventAndData callback);
const UnityRenderTarget* MDKUnityVulkanEventTarget();
