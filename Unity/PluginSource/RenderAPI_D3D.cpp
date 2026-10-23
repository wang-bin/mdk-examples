/*
 * Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
#include "MDKUnityPlugin.h"

#if defined(_WIN32)

#include "Unity/IUnityGraphicsD3D11.h"
#include "Unity/IUnityGraphicsD3D12.h"

#include <d3d11_1.h>
#include <windows.h>
#include <wrl/client.h>

#include <utility>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace {

IUnityGraphicsD3D11* g_d3d11 = nullptr;
ComPtr<ID3D11DeviceContext1> g_d3d11Context1;
ComPtr<ID3DDeviceContextState> g_d3d11PluginContextState;
IUnityGraphicsD3D12v7* g_d3d12v7 = nullptr;
IUnityGraphicsD3D12v6* g_d3d12v6 = nullptr;
IUnityGraphicsD3D12v5* g_d3d12v5 = nullptr;
IUnityGraphicsD3D12v2* g_d3d12v2 = nullptr;
IUnityGraphicsD3D12* g_d3d12 = nullptr;

struct D3D11State {
    ComPtr<ID3D11RenderTargetView> targetView;
};

struct D3D12State {
    MDKUnityPlayer* player = nullptr;
    ID3D12Resource* resource = nullptr;
    ID3D12GraphicsCommandList* commandList = nullptr;
    ComPtr<ID3D12DescriptorHeap> targetViewHeap;
    bool usesUnityCommandList = false;
    bool usesPluginCommandList = false;
    struct PluginCommandList {
        ComPtr<ID3D12CommandAllocator> allocator;
        ComPtr<ID3D12GraphicsCommandList> list;
        UINT64 fence = 0;
    };
    std::vector<PluginCommandList> pluginCommandLists;
    PluginCommandList* activePluginCommandList = nullptr;
};

bool EnsureD3D11ContextState()
{
    if (g_d3d11Context1.Get() && g_d3d11PluginContextState.Get())
        return true;
    if (!g_d3d11 || !g_d3d11->GetDevice())
        return false;

    auto* device = g_d3d11->GetDevice();
    ComPtr<ID3D11Device1> device1;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&device1))))
        return false;

    ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(&context);
    if (!context || FAILED(context.As(&g_d3d11Context1)))
        return false;

    const auto featureLevel = device->GetFeatureLevel();
    if (FAILED(device1->CreateDeviceContextState(0, &featureLevel, 1, D3D11_SDK_VERSION,
                                                  __uuidof(ID3D11Device1), nullptr,
                                                  &g_d3d11PluginContextState))) {
        g_d3d11Context1.Reset();
        return false;
    }
    return true;
}

class D3D11ContextStateGuard final {
public:
    explicit D3D11ContextStateGuard(bool enabled)
    {
        if (!enabled || !EnsureD3D11ContextState())
            return;
        ID3DDeviceContextState* previous = nullptr;
        g_d3d11Context1->SwapDeviceContextState(g_d3d11PluginContextState.Get(), &previous);
        previous_.Attach(previous);
        swapped_ = true;
    }

    ~D3D11ContextStateGuard()
    {
        if (!swapped_ || !g_d3d11Context1)
            return;
        ComPtr<ID3DDeviceContextState> unused;
        g_d3d11Context1->SwapDeviceContextState(previous_.Get(), unused.GetAddressOf());
    }

    D3D11ContextStateGuard(const D3D11ContextStateGuard&) = delete;
    D3D11ContextStateGuard& operator=(const D3D11ContextStateGuard&) = delete;

private:
    ComPtr<ID3DDeviceContextState> previous_;
    bool swapped_ = false;
};

UnityRenderTarget CurrentTarget(const MDKUnityPlayer& player)
{
    return player.renderingTarget;
}

ID3D12Device* D3D12Device()
{
    if (g_d3d12v7)
        return g_d3d12v7->GetDevice();
    if (g_d3d12v6)
        return g_d3d12v6->GetDevice();
    if (g_d3d12v5)
        return g_d3d12v5->GetDevice();
    if (g_d3d12v2)
        return g_d3d12v2->GetDevice();
    return g_d3d12 ? g_d3d12->GetDevice() : nullptr;
}

ID3D12CommandQueue* D3D12Queue()
{
    if (g_d3d12v7)
        return g_d3d12v7->GetCommandQueue();
    if (g_d3d12v6)
        return g_d3d12v6->GetCommandQueue();
    if (g_d3d12v5)
        return g_d3d12v5->GetCommandQueue();
    return g_d3d12 ? g_d3d12->GetCommandQueue() : nullptr;
}

ID3D12Resource* ResolveD3D12Texture(const UnityRenderTarget& target)
{
// GetNativeTexturePtr returns ID3D12Resource*, not a UnityTextureID.
    return reinterpret_cast<ID3D12Resource*>(target.nativeTexture);
}

ID3D12GraphicsCommandList* CurrentD3D12CommandList(D3D12State* state)
{
    if (state->commandList)
        return state->commandList;
    UnityGraphicsD3D12RecordingState recording{};
    if (g_d3d12v7 && g_d3d12v7->CommandRecordingState && g_d3d12v7->CommandRecordingState(&recording))
        state->commandList = recording.commandList;
    else if (g_d3d12v6 && g_d3d12v6->CommandRecordingState && g_d3d12v6->CommandRecordingState(&recording))
        state->commandList = recording.commandList;
    return state->commandList;
}

D3D12State::PluginCommandList* AcquirePluginCommandList(D3D12State* state)
{
    if (!state || !state->usesPluginCommandList)
        return nullptr;
    if (state->activePluginCommandList)
        return state->activePluginCommandList;
    ID3D12Device* device = D3D12Device();
    if (!device)
        return nullptr;
    UINT64 completed = 0;
    if (g_d3d12v5 && g_d3d12v5->GetFrameFence) {
        if (auto* fence = g_d3d12v5->GetFrameFence())
            completed = fence->GetCompletedValue();
    } else if (g_d3d12v2 && g_d3d12v2->GetFrameFence) {
        if (auto* fence = g_d3d12v2->GetFrameFence())
            completed = fence->GetCompletedValue();
    }
    for (auto& entry : state->pluginCommandLists) {
        if (!entry.list || entry.fence > completed)
            continue;
        if (FAILED(entry.allocator->Reset()) || FAILED(entry.list->Reset(entry.allocator.Get(), nullptr)))
            return nullptr;
        state->activePluginCommandList = &entry;
        return &entry;
    }
    D3D12State::PluginCommandList entry;
    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                               IID_PPV_ARGS(&entry.allocator))))
        return nullptr;
    if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, entry.allocator.Get(), nullptr,
                                         IID_PPV_ARGS(&entry.list))))
        return nullptr;
    if (FAILED(entry.list->Close()))
        return nullptr;
    if (FAILED(entry.allocator->Reset()) || FAILED(entry.list->Reset(entry.allocator.Get(), nullptr)))
        return nullptr;
    state->pluginCommandLists.push_back(std::move(entry));
    state->activePluginCommandList = &state->pluginCommandLists.back();
    return state->activePluginCommandList;
}

ID3D12GraphicsCommandList* CurrentPluginCommandList(const void* opaque)
{
    auto* state = const_cast<D3D12State*>(static_cast<const D3D12State*>(opaque));
    const auto entry = AcquirePluginCommandList(state);
    return entry ? entry->list.Get() : nullptr;
}

void SubmitPluginCommandList(D3D12State* state)
{
    if (!state || !state->activePluginCommandList)
        return;
    auto* entry = state->activePluginCommandList;
    state->activePluginCommandList = nullptr;
    if (FAILED(entry->list->Close()))
        return;
    UnityGraphicsD3D12ResourceState resourceState{
        .resource = state->resource,
        .expected = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .current = D3D12_RESOURCE_STATE_RENDER_TARGET,
    };
    UINT64 fence = 0;
    if (g_d3d12v5 && g_d3d12v5->ExecuteCommandList)
        fence = g_d3d12v5->ExecuteCommandList(entry->list.Get(), state->resource ? 1 : 0, &resourceState);
    else if (g_d3d12v2 && g_d3d12v2->ExecuteCommandList)
        fence = g_d3d12v2->ExecuteCommandList(entry->list.Get(), state->resource ? 1 : 0, &resourceState);
    entry->fence = fence;
}

ID3D12Resource* CurrentRenderTarget(const void* opaque, UINT* index, UINT* count, D3D12_RESOURCE_STATES* state)
{
    [[maybe_unused]] const auto* d3d = static_cast<const D3D12State*>(opaque);
    if (index)
        *index = 0;
    if (count)
        *count = 1;
    if (state)
        *state = D3D12_RESOURCE_STATE_RENDER_TARGET;
// Unity manages resource barriers. The supplied RTV and typed colorFormat describe the foreign render pass.
    return nullptr;
}

ID3D12GraphicsCommandList* CurrentCommandList(const void* opaque)
{
    return CurrentD3D12CommandList(const_cast<D3D12State*>(static_cast<const D3D12State*>(opaque)));
}

} // namespace

bool MDKUnityD3DInitialize(MDKUnityPlayer& player, UnityGfxRenderer renderer)
{
    const auto target = CurrentTarget(player);
    if (!target.nativeTexture)
        return false;

    if (renderer == kUnityGfxRendererD3D11) {
        // Unity normally populates this cache from the graphics-device
        // initialize callback.  A managed P/Invoke can happen after that
        // callback in the Editor, so resolve the active interface lazily as a
        // safe fallback (unlike D3D12 ConfigureEvent, this query has no side
        // effects on another renderer).
        if (!g_d3d11) {
            if (auto* interfaces = MDKUnityInterfaces())
                g_d3d11 = interfaces->Get<IUnityGraphicsD3D11>();
        }
        if (!g_d3d11 || !g_d3d11->GetDevice())
            return false;
        EnsureD3D11ContextState();
        auto state = std::make_unique<D3D11State>();
        auto* texture = static_cast<ID3D11Texture2D*>(target.nativeTexture);
        D3D11_TEXTURE2D_DESC desc{};
        texture->GetDesc(&desc);
        auto format = desc.Format;
// Unity can allocate typeless resources; MDK's texture shortcut cannot use a typeless format for an RTV.
        switch (format) {
        case DXGI_FORMAT_R16G16B16A16_TYPELESS: format = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS: format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS: format = DXGI_FORMAT_B8G8R8A8_UNORM; break;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS: format = DXGI_FORMAT_B8G8R8X8_UNORM; break;
        default: break;
        }
        if (target.outputMode == 1) {
            UINT support = 0;
            if (format != DXGI_FORMAT_R16G16B16A16_FLOAT ||
                FAILED(g_d3d11->GetDevice()->CheckFormatSupport(format, &support)) ||
                !(support & D3D11_FORMAT_SUPPORT_RENDER_TARGET) || !(support & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE))
                return player.renderFailure("D3D11 HDRLinear requires a supported RGBA16F render target");
        }
        const D3D11_RENDER_TARGET_VIEW_DESC viewDesc{
            .Format = format,
            .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
            .Texture2D = {.MipSlice = 0},
        };
        if (FAILED(g_d3d11->GetDevice()->CreateRenderTargetView(texture, &viewDesc, &state->targetView)))
            return false;
        auto api = std::make_unique<mdk::D3D11RenderAPI>();
        api->rtv = state->targetView.Get();
        player.backendState = state.release();
        player.renderApi = std::move(api);
        player.player.setRenderAPI(player.renderApi.get(), &player);
        return true;
    }
    if (renderer != kUnityGfxRendererD3D12)
        return false;

    auto* device = D3D12Device();
    auto* queue = D3D12Queue();
    if (!device || !queue)
        return false;
    auto state = std::make_unique<D3D12State>();
    state->player = &player;
    state->resource = ResolveD3D12Texture(target);
    if (!state->resource)
        return false;
    state->usesUnityCommandList = g_d3d12v7 || g_d3d12v6;
    state->usesPluginCommandList = !state->usesUnityCommandList && (g_d3d12v5 || g_d3d12v2);
    if (!state->usesUnityCommandList && !state->usesPluginCommandList)
        return false;
    auto format = state->resource->GetDesc().Format;
    switch (format) {
    case DXGI_FORMAT_R16G16B16A16_TYPELESS: format = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS: format = DXGI_FORMAT_B8G8R8A8_UNORM; break;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS: format = DXGI_FORMAT_B8G8R8X8_UNORM; break;
    default: break;
    }
    if (target.outputMode == 1) {
        D3D12_FEATURE_DATA_FORMAT_SUPPORT support{.Format = format};
        if (format != DXGI_FORMAT_R16G16B16A16_FLOAT ||
            FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &support, sizeof(support))) ||
            !(support.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) ||
            !(support.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE))
            return player.renderFailure("D3D12 HDRLinear requires a supported RGBA16F render target");
    }
    const D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = 1,
    };
    if (FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&state->targetViewHeap))))
        return false;
    const D3D12_RENDER_TARGET_VIEW_DESC viewDesc{
        .Format = format,
        .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
        .Texture2D = {.MipSlice = 0, .PlaneSlice = 0},
    };
    const auto targetView = state->targetViewHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateRenderTargetView(state->resource, &viewDesc, targetView);
    auto api = std::make_unique<mdk::D3D12RenderAPI>();
    api->cmdQueue = queue;
    api->rt = state->resource;
    api->rtvHandle = targetView;
    api->opaque = state.get();
    api->currentRenderTarget = CurrentRenderTarget;
    api->currentCommandList = state->usesUnityCommandList ? CurrentCommandList
        : (state->usesPluginCommandList ? CurrentPluginCommandList : nullptr);
    api->colorFormat = format;
    const auto usesUnityCommandList = state->usesUnityCommandList;
    player.backendState = state.release();
    player.renderApi = std::move(api);
    player.player.setRenderAPI(player.renderApi.get(), &player);
    if (usesUnityCommandList)
        (void)CurrentD3D12CommandList(static_cast<D3D12State*>(player.backendState));
    return true;
}

bool MDKUnityD3DRender(MDKUnityPlayer& player, UnityGfxRenderer renderer)
{
    if ((renderer != kUnityGfxRendererD3D11 && renderer != kUnityGfxRendererD3D12) || !player.renderApi)
        return false;
    D3D12State* state = nullptr;
    if (renderer == kUnityGfxRendererD3D12 && player.backendState) {
        state = static_cast<D3D12State*>(player.backendState);
        const auto target = CurrentTarget(player);
        state->resource = ResolveD3D12Texture(target);
        state->commandList = nullptr;
        if (state->usesUnityCommandList)
            (void)CurrentD3D12CommandList(state);
        auto* api = static_cast<mdk::D3D12RenderAPI*>(player.renderApi.get());
        api->rt = state->resource;
    }
    D3D11ContextStateGuard d3d11StateGuard(renderer == kUnityGfxRendererD3D11);
    const auto result = player.player.renderVideo(&player);
    if (state && state->usesPluginCommandList)
        SubmitPluginCommandList(state);
    return result >= 0;
}

void MDKUnityD3DRelease(MDKUnityPlayer& player, UnityGfxRenderer renderer)
{
    if (renderer == kUnityGfxRendererD3D11) {
        delete static_cast<D3D11State*>(player.backendState);
        player.backendState = nullptr;
    } else if (renderer == kUnityGfxRendererD3D12) {
        delete static_cast<D3D12State*>(player.backendState);
        player.backendState = nullptr;
    }
}

void MDKUnityD3DDeviceEvent(UnityGfxDeviceEventType eventType, IUnityInterfaces* interfaces,
                             UnityGfxRenderer renderer)
{
    if ((eventType == kUnityGfxDeviceEventInitialize || eventType == kUnityGfxDeviceEventAfterReset) && interfaces) {
        // Some Unity-compatible editors expose every graphics interface from
        // IUnityInterfaces, even when another renderer is active.  ConfigureEvent
        // is renderer-specific; calling the D3D12 entry point while D3D11 is
        // active can crash the editor during the first P/Invoke load.
        g_d3d11 = renderer == kUnityGfxRendererD3D11 ? interfaces->Get<IUnityGraphicsD3D11>() : nullptr;
        if (renderer == kUnityGfxRendererD3D12) {
            g_d3d12v7 = interfaces->Get<IUnityGraphicsD3D12v7>();
            g_d3d12v6 = interfaces->Get<IUnityGraphicsD3D12v6>();
            g_d3d12v5 = interfaces->Get<IUnityGraphicsD3D12v5>();
            g_d3d12v2 = interfaces->Get<IUnityGraphicsD3D12v2>();
            g_d3d12 = interfaces->Get<IUnityGraphicsD3D12>();
            UnityD3D12PluginEventConfig config{
                .graphicsQueueAccess = kUnityD3D12GraphicsQueueAccess_DontCare,
                .flags = kUnityD3D12EventConfigFlag_SyncWorkerThreads | kUnityD3D12EventConfigFlag_ModifiesCommandBuffersState,
                .ensureActiveRenderTextureIsBound = true,
            };
            if (g_d3d12v7 && g_d3d12v7->ConfigureEvent)
                g_d3d12v7->ConfigureEvent(1, &config);
            else if (g_d3d12v6 && g_d3d12v6->ConfigureEvent)
                g_d3d12v6->ConfigureEvent(1, &config);
        } else {
            g_d3d12v7 = nullptr;
            g_d3d12v6 = nullptr;
            g_d3d12v5 = nullptr;
            g_d3d12v2 = nullptr;
            g_d3d12 = nullptr;
        }
    } else if (eventType == kUnityGfxDeviceEventShutdown || eventType == kUnityGfxDeviceEventBeforeReset) {
        g_d3d11 = nullptr;
        g_d3d11Context1.Reset();
        g_d3d11PluginContextState.Reset();
        g_d3d12v7 = nullptr;
        g_d3d12v6 = nullptr;
        g_d3d12v5 = nullptr;
        g_d3d12v2 = nullptr;
        g_d3d12 = nullptr;
    }
}

#else

bool MDKUnityD3DInitialize(MDKUnityPlayer&, UnityGfxRenderer) { return false; }
bool MDKUnityD3DRender(MDKUnityPlayer&, UnityGfxRenderer) { return false; }
void MDKUnityD3DRelease(MDKUnityPlayer&, UnityGfxRenderer) {}
void MDKUnityD3DDeviceEvent(UnityGfxDeviceEventType, IUnityInterfaces*, UnityGfxRenderer) {}

#endif
