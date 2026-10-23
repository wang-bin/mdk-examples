/*
 * Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
#include "MDKUnityPlugin.h"

#if defined(__APPLE__) && defined(__OBJC__)

#include "Unity/IUnityGraphicsMetal.h"
#import <Metal/Metal.h>

namespace {

IUnityGraphicsMetal* g_metal = nullptr;

struct MetalState {
    id<MTLTexture> texture = nil;
};

const void* CurrentRenderTarget(const void* opaque)
{
    const auto* state = static_cast<const MetalState*>(opaque);
    return (__bridge const void*)state->texture;
}

void CurrentCommand(const void** encoder, const void** commandBuffer, const void*)
{
    if (!g_metal)
        return;
    const auto enc = g_metal->CurrentCommandEncoder();
    const auto buf = g_metal->CurrentCommandBuffer();
    if (encoder)
        *encoder = (__bridge const void*)enc;
    if (commandBuffer)
        *commandBuffer = (__bridge const void*)buf;
}

} // namespace

bool MDKUnityMetalInitialize(MDKUnityPlayer& player, UnityGfxRenderer renderer)
{
    UnityRenderTarget target;
    {
        const std::lock_guard lock(player.targetMutex);
        target = player.target;
    }
    if (renderer != kUnityGfxRendererMetal || !g_metal || !g_metal->MetalDevice() || !target.nativeTexture)
        return false;
    auto state = std::make_unique<MetalState>();
    state->texture = (__bridge id<MTLTexture>)target.nativeTexture;
    if (!state->texture)
        return false;
    auto api = std::make_unique<mdk::MetalRenderAPI>();
    api->device = (__bridge const void*)g_metal->MetalDevice();
    api->texture = (__bridge const void*)state->texture;
    api->opaque = state.get();
    api->currentRenderTarget = CurrentRenderTarget;
    api->currentCommand = CurrentCommand;
    api->colorFormat = static_cast<unsigned>(state->texture.pixelFormat);
    player.backendState = state.release();
    player.renderApi = std::move(api);
    player.player.setRenderAPI(player.renderApi.get(), &player);
    return true;
}

bool MDKUnityMetalRender(MDKUnityPlayer& player, UnityGfxRenderer renderer)
{
    if (renderer != kUnityGfxRendererMetal || !player.renderApi)
        return false;
    return player.player.renderVideo(&player) >= 0;
}

void MDKUnityMetalRelease(MDKUnityPlayer& player, UnityGfxRenderer renderer)
{
    if (renderer != kUnityGfxRendererMetal)
        return;
    delete static_cast<MetalState*>(player.backendState);
    player.backendState = nullptr;
}

void MDKUnityMetalDeviceEvent(UnityGfxDeviceEventType eventType, IUnityInterfaces* interfaces,
                              UnityGfxRenderer renderer)
{
    if ((eventType == kUnityGfxDeviceEventInitialize || eventType == kUnityGfxDeviceEventAfterReset)
        && interfaces && renderer == kUnityGfxRendererMetal)
        g_metal = interfaces->Get<IUnityGraphicsMetal>();
    else if (eventType == kUnityGfxDeviceEventShutdown || eventType == kUnityGfxDeviceEventBeforeReset)
        g_metal = nullptr;
}

#else

bool MDKUnityMetalInitialize(MDKUnityPlayer&, UnityGfxRenderer) { return false; }
bool MDKUnityMetalRender(MDKUnityPlayer&, UnityGfxRenderer) { return false; }
void MDKUnityMetalRelease(MDKUnityPlayer&, UnityGfxRenderer) {}
void MDKUnityMetalDeviceEvent(UnityGfxDeviceEventType, IUnityInterfaces*, UnityGfxRenderer) {}

#endif
