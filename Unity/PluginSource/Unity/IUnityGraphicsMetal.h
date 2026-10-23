/* Unity Companion License: https://unity3d.com/legal/licenses/Unity_Companion_License */
#pragma once
#include "IUnityInterface.h"
#if defined(__OBJC__)
@class NSBundle;
@protocol MTLDevice;
@protocol MTLCommandBuffer;
@protocol MTLCommandEncoder;
@protocol MTLTexture;
UNITY_DECLARE_INTERFACE(IUnityGraphicsMetal) {
    NSBundle* (UNITY_INTERFACE_API *MetalBundle)();
    id<MTLDevice> (UNITY_INTERFACE_API *MetalDevice)();
    id<MTLCommandBuffer> (UNITY_INTERFACE_API *CurrentCommandBuffer)();
    id<MTLCommandEncoder> (UNITY_INTERFACE_API *CurrentCommandEncoder)();
    void (UNITY_INTERFACE_API *EndCurrentCommandEncoder)();
};
UNITY_REGISTER_INTERFACE_GUID(0x992C8EAEA95811E5ULL, 0x9A62C4B5B9876117ULL, IUnityGraphicsMetal)
#endif
