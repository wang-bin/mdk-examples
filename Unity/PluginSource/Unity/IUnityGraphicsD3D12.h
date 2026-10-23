/* Unity Companion License: https://unity3d.com/legal/licenses/Unity_Companion_License */
#pragma once
#include "IUnityInterface.h"
#ifdef _WIN32
#include <d3d12.h>
#include <dxgiformat.h>

struct UnityGraphicsD3D12ResourceState {
    ID3D12Resource* resource;
    D3D12_RESOURCE_STATES expected;
    D3D12_RESOURCE_STATES current;
};
struct UnityGraphicsD3D12RecordingState { ID3D12GraphicsCommandList* commandList; };
enum UnityD3D12GraphicsQueueAccess { kUnityD3D12GraphicsQueueAccess_DontCare, kUnityD3D12GraphicsQueueAccess_Allow };
enum UnityD3D12EventConfigFlagBits {
    kUnityD3D12EventConfigFlag_EnsurePreviousFrameSubmission = 1 << 0,
    kUnityD3D12EventConfigFlag_FlushCommandBuffers = 1 << 1,
    kUnityD3D12EventConfigFlag_SyncWorkerThreads = 1 << 2,
    kUnityD3D12EventConfigFlag_ModifiesCommandBuffersState = 1 << 3,
};
struct UnityD3D12PluginEventConfig {
    UnityD3D12GraphicsQueueAccess graphicsQueueAccess;
    UINT32 flags;
    bool ensureActiveRenderTextureIsBound;
};

UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D12v7) {
    ID3D12Device* (UNITY_INTERFACE_API *GetDevice)();
    IDXGISwapChain* (UNITY_INTERFACE_API *GetSwapChain)();
    UINT32 (UNITY_INTERFACE_API *GetSyncInterval)();
    UINT (UNITY_INTERFACE_API *GetPresentFlags)();
    ID3D12Fence* (UNITY_INTERFACE_API *GetFrameFence)();
    UINT64 (UNITY_INTERFACE_API *GetNextFrameFenceValue)();
    UINT64 (UNITY_INTERFACE_API *ExecuteCommandList)(ID3D12GraphicsCommandList*, int, UnityGraphicsD3D12ResourceState*);
    void (UNITY_INTERFACE_API *SetPhysicalVideoMemoryControlValues)(const void*);
    ID3D12CommandQueue* (UNITY_INTERFACE_API *GetCommandQueue)();
    ID3D12Resource* (UNITY_INTERFACE_API *TextureFromRenderBuffer)(void*);
    ID3D12Resource* (UNITY_INTERFACE_API *TextureFromNativeTexture)(unsigned int);
    void (UNITY_INTERFACE_API *ConfigureEvent)(int, const UnityD3D12PluginEventConfig*);
    bool (UNITY_INTERFACE_API *CommandRecordingState)(UnityGraphicsD3D12RecordingState*);
};
UNITY_REGISTER_INTERFACE_GUID(0x4624B0DA41B64AACULL, 0x915AABCB9BC3F0D3ULL, IUnityGraphicsD3D12v7)

UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D12v6) {
    ID3D12Device* (UNITY_INTERFACE_API *GetDevice)();
    ID3D12Fence* (UNITY_INTERFACE_API *GetFrameFence)();
    UINT64 (UNITY_INTERFACE_API *GetNextFrameFenceValue)();
    UINT64 (UNITY_INTERFACE_API *ExecuteCommandList)(ID3D12GraphicsCommandList*, int, UnityGraphicsD3D12ResourceState*);
    void (UNITY_INTERFACE_API *SetPhysicalVideoMemoryControlValues)(const void*);
    ID3D12CommandQueue* (UNITY_INTERFACE_API *GetCommandQueue)();
    ID3D12Resource* (UNITY_INTERFACE_API *TextureFromRenderBuffer)(void*);
    ID3D12Resource* (UNITY_INTERFACE_API *TextureFromNativeTexture)(unsigned int);
    void (UNITY_INTERFACE_API *ConfigureEvent)(int, const UnityD3D12PluginEventConfig*);
    bool (UNITY_INTERFACE_API *CommandRecordingState)(UnityGraphicsD3D12RecordingState*);
};
UNITY_REGISTER_INTERFACE_GUID(0xA396DCE58CAC4D78ULL, 0xAFDD9B281F20B840ULL, IUnityGraphicsD3D12v6)

UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D12v5) {
    ID3D12Device* (UNITY_INTERFACE_API *GetDevice)();
    ID3D12Fence* (UNITY_INTERFACE_API *GetFrameFence)();
    UINT64 (UNITY_INTERFACE_API *GetNextFrameFenceValue)();
    UINT64 (UNITY_INTERFACE_API *ExecuteCommandList)(ID3D12GraphicsCommandList*, int, UnityGraphicsD3D12ResourceState*);
    void (UNITY_INTERFACE_API *SetPhysicalVideoMemoryControlValues)(const void*);
    ID3D12CommandQueue* (UNITY_INTERFACE_API *GetCommandQueue)();
    ID3D12Resource* (UNITY_INTERFACE_API *TextureFromRenderBuffer)(void*);
};
UNITY_REGISTER_INTERFACE_GUID(0xF5C8D8A37D37BC42ULL, 0xB02DFE93B5064A27ULL, IUnityGraphicsD3D12v5)

UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D12v2) {
    ID3D12Device* (UNITY_INTERFACE_API *GetDevice)();
    ID3D12Fence* (UNITY_INTERFACE_API *GetFrameFence)();
    UINT64 (UNITY_INTERFACE_API *GetNextFrameFenceValue)();
    UINT64 (UNITY_INTERFACE_API *ExecuteCommandList)(ID3D12GraphicsCommandList*, int, UnityGraphicsD3D12ResourceState*);
};
UNITY_REGISTER_INTERFACE_GUID(0xEC39D2F18446C745ULL, 0xB1A2626641D6B11FULL, IUnityGraphicsD3D12v2)

UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D12) {
    ID3D12Device* (UNITY_INTERFACE_API *GetDevice)();
    ID3D12CommandQueue* (UNITY_INTERFACE_API *GetCommandQueue)();
    ID3D12Fence* (UNITY_INTERFACE_API *GetFrameFence)();
    UINT64 (UNITY_INTERFACE_API *GetNextFrameFenceValue)();
    bool (UNITY_INTERFACE_API *GetResourceState)(ID3D12Resource*, D3D12_RESOURCE_STATES*);
    void (UNITY_INTERFACE_API *SetResourceState)(ID3D12Resource*, D3D12_RESOURCE_STATES);
};
UNITY_REGISTER_INTERFACE_GUID(0xEF4CEC88A45F4C4CULL, 0xBD295B6F2A38D9DEULL, IUnityGraphicsD3D12)
#endif
