/*
 * Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
#include "MDKUnityPlugin.h"

bool MDKUnityOpenGLInitialize(MDKUnityPlayer&, UnityGfxRenderer);
bool MDKUnityOpenGLRender(MDKUnityPlayer&, UnityGfxRenderer);
void MDKUnityOpenGLRelease(MDKUnityPlayer&, UnityGfxRenderer);
void MDKUnityOpenGLDeviceEvent(UnityGfxDeviceEventType, IUnityInterfaces*, UnityGfxRenderer);

bool MDKUnityD3DInitialize(MDKUnityPlayer&, UnityGfxRenderer);
bool MDKUnityD3DRender(MDKUnityPlayer&, UnityGfxRenderer);
void MDKUnityD3DRelease(MDKUnityPlayer&, UnityGfxRenderer);
void MDKUnityD3DDeviceEvent(UnityGfxDeviceEventType, IUnityInterfaces*, UnityGfxRenderer);

#if !defined(_WIN32)
bool MDKUnityD3DInitialize(MDKUnityPlayer&, UnityGfxRenderer) { return false; }
bool MDKUnityD3DRender(MDKUnityPlayer&, UnityGfxRenderer) { return false; }
void MDKUnityD3DRelease(MDKUnityPlayer&, UnityGfxRenderer) {}
void MDKUnityD3DDeviceEvent(UnityGfxDeviceEventType, IUnityInterfaces*, UnityGfxRenderer) {}
#endif

bool MDKUnityVulkanInitialize(MDKUnityPlayer&, UnityGfxRenderer);
bool MDKUnityVulkanRender(MDKUnityPlayer&, UnityGfxRenderer);
void MDKUnityVulkanRelease(MDKUnityPlayer&, UnityGfxRenderer);
void MDKUnityVulkanDeviceEvent(UnityGfxDeviceEventType, IUnityInterfaces*, UnityGfxRenderer);

#if !defined(_WIN32) && !defined(__linux__) && !defined(__ANDROID__)
bool MDKUnityVulkanDispatchEvent(int, const std::shared_ptr<MDKUnityPlayer>&, UnityRenderingEventAndData) { return false; }
const UnityRenderTarget* MDKUnityVulkanEventTarget() { return nullptr; }
bool MDKUnityVulkanInitialize(MDKUnityPlayer&, UnityGfxRenderer) { return false; }
bool MDKUnityVulkanRender(MDKUnityPlayer&, UnityGfxRenderer) { return false; }
void MDKUnityVulkanRelease(MDKUnityPlayer&, UnityGfxRenderer) {}
void MDKUnityVulkanDeviceEvent(UnityGfxDeviceEventType, IUnityInterfaces*, UnityGfxRenderer) {}
#endif

bool MDKUnityMetalInitialize(MDKUnityPlayer&, UnityGfxRenderer);
bool MDKUnityMetalRender(MDKUnityPlayer&, UnityGfxRenderer);
void MDKUnityMetalRelease(MDKUnityPlayer&, UnityGfxRenderer);
void MDKUnityMetalDeviceEvent(UnityGfxDeviceEventType, IUnityInterfaces*, UnityGfxRenderer);

#if !defined(__APPLE__)
bool MDKUnityMetalInitialize(MDKUnityPlayer&, UnityGfxRenderer) { return false; }
bool MDKUnityMetalRender(MDKUnityPlayer&, UnityGfxRenderer) { return false; }
void MDKUnityMetalRelease(MDKUnityPlayer&, UnityGfxRenderer) {}
void MDKUnityMetalDeviceEvent(UnityGfxDeviceEventType, IUnityInterfaces*, UnityGfxRenderer) {}
#endif

bool MDKUnityBackendInitialize(MDKUnityPlayer& player, UnityGfxRenderer renderer)
{
    switch (renderer) {
    case kUnityGfxRendererD3D11:
    case kUnityGfxRendererD3D12:
        return MDKUnityD3DInitialize(player, renderer);
    case kUnityGfxRendererVulkan:
        return MDKUnityVulkanInitialize(player, renderer);
    case kUnityGfxRendererMetal:
        return MDKUnityMetalInitialize(player, renderer);
    case kUnityGfxRendererOpenGL:
    case kUnityGfxRendererOpenGLCore:
    case kUnityGfxRendererOpenGLES20:
    case kUnityGfxRendererOpenGLES30:
        return MDKUnityOpenGLInitialize(player, renderer);
    default:
        return false;
    }
}

bool MDKUnityBackendRender(MDKUnityPlayer& player, UnityGfxRenderer renderer)
{
    switch (renderer) {
    case kUnityGfxRendererD3D11:
    case kUnityGfxRendererD3D12:
        return MDKUnityD3DRender(player, renderer);
    case kUnityGfxRendererVulkan:
        return MDKUnityVulkanRender(player, renderer);
    case kUnityGfxRendererMetal:
        return MDKUnityMetalRender(player, renderer);
    case kUnityGfxRendererOpenGL:
    case kUnityGfxRendererOpenGLCore:
    case kUnityGfxRendererOpenGLES20:
    case kUnityGfxRendererOpenGLES30:
        return MDKUnityOpenGLRender(player, renderer);
    default:
        return false;
    }
}

void MDKUnityBackendRelease(MDKUnityPlayer& player, UnityGfxRenderer renderer)
{
    switch (renderer) {
    case kUnityGfxRendererD3D11:
    case kUnityGfxRendererD3D12:
        MDKUnityD3DRelease(player, renderer);
        break;
    case kUnityGfxRendererVulkan:
        MDKUnityVulkanRelease(player, renderer);
        break;
    case kUnityGfxRendererMetal:
        MDKUnityMetalRelease(player, renderer);
        break;
    case kUnityGfxRendererOpenGL:
    case kUnityGfxRendererOpenGLCore:
    case kUnityGfxRendererOpenGLES20:
    case kUnityGfxRendererOpenGLES30:
        MDKUnityOpenGLRelease(player, renderer);
        break;
    default:
        break;
    }
}

void MDKUnityBackendDeviceEvent(UnityGfxDeviceEventType eventType, IUnityInterfaces* interfaces,
                                UnityGfxRenderer renderer)
{
    MDKUnityOpenGLDeviceEvent(eventType, interfaces, renderer);
    MDKUnityD3DDeviceEvent(eventType, interfaces, renderer);
    MDKUnityVulkanDeviceEvent(eventType, interfaces, renderer);
    MDKUnityMetalDeviceEvent(eventType, interfaces, renderer);
}

const char* MDKUnityBackendName(UnityGfxRenderer renderer)
{
    switch (renderer) {
    case kUnityGfxRendererD3D11: return "D3D11";
    case kUnityGfxRendererD3D12: return "D3D12";
    case kUnityGfxRendererVulkan: return "Vulkan";
    case kUnityGfxRendererMetal: return "Metal";
    case kUnityGfxRendererOpenGL:
    case kUnityGfxRendererOpenGLCore: return "OpenGL";
    case kUnityGfxRendererOpenGLES20:
    case kUnityGfxRendererOpenGLES30: return "OpenGL ES";
    default: return "Unsupported";
    }
}
