/*
 * Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
#include "MDKUnityPlugin.h"

#if defined(_WIN32) || defined(__linux__) || defined(__ANDROID__)
#include "Unity/IUnityGraphicsVulkan.h"

namespace {

IUnityGraphicsVulkanV2* g_vk2 = nullptr;
IUnityGraphicsVulkan* g_vk1 = nullptr;

struct VulkanEvent {
    std::shared_ptr<MDKUnityPlayer> player;
    UnityRenderingEventAndData callback = nullptr;
    UnityRenderTarget target;
    UnityVulkanImage image{};
};

thread_local const VulkanEvent* g_queueEvent = nullptr;

struct VulkanState {
    MDKUnityPlayer* player = nullptr;
    VkImage image = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    int width = 0;
    int height = 0;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBuffer commands = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    PFN_vkCreateCommandPool createCommandPool = nullptr;
    PFN_vkDestroyCommandPool destroyCommandPool = nullptr;
    PFN_vkAllocateCommandBuffers allocateCommandBuffers = nullptr;
    PFN_vkResetCommandPool resetCommandPool = nullptr;
    PFN_vkBeginCommandBuffer beginCommandBuffer = nullptr;
    PFN_vkEndCommandBuffer endCommandBuffer = nullptr;
    PFN_vkCreateFence createFence = nullptr;
    PFN_vkDestroyFence destroyFence = nullptr;
    PFN_vkResetFences resetFences = nullptr;
    PFN_vkQueueSubmit queueSubmit = nullptr;
    PFN_vkWaitForFences waitForFences = nullptr;

    ~VulkanState()
    {
        if (fence)
            destroyFence(device, fence, nullptr);
        if (pool)
            destroyCommandPool(device, pool, nullptr);
    }
};

bool AcquireTexture(MDKUnityPlayer& player, VulkanState& state)
{
// Resource access must happen before AccessQueue, never inside its callback.
    if (!g_queueEvent || g_queueEvent->player.get() != &player)
        return false;
    const auto& image = g_queueEvent->image;
    state.player = &player;
    state.image = image.image;
    state.format = image.format;
    state.width = static_cast<int>(image.extent.width);
    state.height = static_cast<int>(image.extent.height);
    return state.image != VK_NULL_HANDLE && state.width > 0 && state.height > 0;
}

int RenderTargetInfo(void* opaque, int* width, int* height, VkFormat* format, VkImageLayout* layout)
{
    const auto* state = static_cast<const VulkanState*>(opaque);
    if (width)
        *width = state->width;
    if (height)
        *height = state->height;
    if (format)
        *format = state->format;
    if (layout)
        *layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    return 1;
}

VkCommandBuffer CurrentCommandBuffer(void* opaque)
{
    return static_cast<VulkanState*>(opaque)->commands;
}

void UNITY_INTERFACE_API OnQueueEvent(int eventId, void* opaque)
{
    const std::unique_ptr<VulkanEvent> event(static_cast<VulkanEvent*>(opaque));
    g_queueEvent = event.get();
    event->callback(eventId, reinterpret_cast<void*>(static_cast<uintptr_t>(event->player->handle())));
    g_queueEvent = nullptr;
}

} // namespace

const UnityRenderTarget* MDKUnityVulkanEventTarget()
{
    return g_queueEvent ? &g_queueEvent->target : nullptr;
}

bool MDKUnityVulkanDispatchEvent(int eventId, const std::shared_ptr<MDKUnityPlayer>& player,
                                 UnityRenderingEventAndData callback)
{
    if (g_queueEvent)
        return false;
    if (!g_vk2 && !g_vk1)
        return true;
    auto event = std::make_unique<VulkanEvent>();
    event->player = player;
    event->callback = callback;
    {
        const std::lock_guard lock(player->targetMutex);
        event->target = player->target;
    }
    if (eventId == 1 && !player->destroyRequested()) {
        const auto access = g_vk2 ? g_vk2->AccessTexture : g_vk1->AccessTexture;
        if (!event->target.nativeTexture ||
            !access(event->target.nativeTexture, UnityVulkanWholeImage,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    kUnityVulkanResourceAccess_PipelineBarrier, &event->image))
            return true;
    }
// Flush Unity's texture barriers before submitting our primary command buffer.
    const auto accessQueue = g_vk2 ? g_vk2->AccessQueue : g_vk1->AccessQueue;
    accessQueue(OnQueueEvent, eventId, event.release(), true);
    return true;
}

bool MDKUnityVulkanInitialize(MDKUnityPlayer& player, UnityGfxRenderer renderer)
{
    if (renderer != kUnityGfxRendererVulkan || (!g_vk2 && !g_vk1))
        return false;
    auto state = std::make_unique<VulkanState>();
    if (!AcquireTexture(player, *state))
        return false;
    const auto instance = g_vk2 ? g_vk2->Instance() : g_vk1->Instance();
    if (instance.device == VK_NULL_HANDLE)
        return false;

    state->device = instance.device;
    state->queue = instance.graphicsQueue;
    const auto getDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
        instance.getInstanceProcAddr(instance.instance, "vkGetDeviceProcAddr"));
    if (!getDeviceProcAddr)
        return false;
#define LOAD_VK(member, name) \
    state->member = reinterpret_cast<PFN_##name>(getDeviceProcAddr(instance.device, #name)); \
    if (!state->member) return false
    LOAD_VK(createCommandPool, vkCreateCommandPool);
    LOAD_VK(destroyCommandPool, vkDestroyCommandPool);
    LOAD_VK(allocateCommandBuffers, vkAllocateCommandBuffers);
    LOAD_VK(resetCommandPool, vkResetCommandPool);
    LOAD_VK(beginCommandBuffer, vkBeginCommandBuffer);
    LOAD_VK(endCommandBuffer, vkEndCommandBuffer);
    LOAD_VK(createFence, vkCreateFence);
    LOAD_VK(destroyFence, vkDestroyFence);
    LOAD_VK(resetFences, vkResetFences);
    LOAD_VK(queueSubmit, vkQueueSubmit);
    LOAD_VK(waitForFences, vkWaitForFences);
#undef LOAD_VK
    const VkCommandPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = instance.queueFamilyIndex,
    };
    if (state->createCommandPool(instance.device, &poolInfo, nullptr, &state->pool) != VK_SUCCESS)
        return false;
    const VkCommandBufferAllocateInfo allocation{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = state->pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    if (state->allocateCommandBuffers(instance.device, &allocation, &state->commands) != VK_SUCCESS)
        return false;
    const VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (state->createFence(instance.device, &fenceInfo, nullptr, &state->fence) != VK_SUCCESS)
        return false;

    auto api = std::make_unique<mdk::VulkanRenderAPI>();
    api->instance = instance.instance;
    api->phy_device = instance.physicalDevice;
    api->device = instance.device;
    api->graphics_queue = instance.graphicsQueue;
    api->graphics_family = static_cast<int>(instance.queueFamilyIndex);
    api->rt = state->image;
    api->opaque = state.get();
    api->renderTargetInfo = RenderTargetInfo;
    api->currentCommandBuffer = CurrentCommandBuffer;
    player.backendState = state.release();
    player.renderApi = std::move(api);
    player.player.setRenderAPI(player.renderApi.get(), &player);
    return true;
}

bool MDKUnityVulkanRender(MDKUnityPlayer& player, UnityGfxRenderer renderer)
{
    if (renderer != kUnityGfxRendererVulkan || !player.renderApi || !player.backendState)
        return false;
    auto* state = static_cast<VulkanState*>(player.backendState);
    const auto oldImage = state->image;
    if (!AcquireTexture(player, *state))
        return false;
    if (state->image != oldImage) {
        auto* api = static_cast<mdk::VulkanRenderAPI*>(player.renderApi.get());
        api->rt = state->image;
        api->renderTargetInfo = RenderTargetInfo;
        player.player.setRenderAPI(player.renderApi.get(), &player);
    }
    if (state->resetCommandPool(state->device, state->pool, 0) != VK_SUCCESS ||
        state->resetFences(state->device, 1, &state->fence) != VK_SUCCESS)
        return false;
    const VkCommandBufferBeginInfo begin{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    if (state->beginCommandBuffer(state->commands, &begin) != VK_SUCCESS)
        return false;
    const bool rendered = player.player.renderVideo(&player) >= 0;
    if (state->endCommandBuffer(state->commands) != VK_SUCCESS)
        return false;
    const VkSubmitInfo submit{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &state->commands,
    };
    if (state->queueSubmit(state->queue, 1, &submit, state->fence) != VK_SUCCESS)
        return false;
// Complete GPU use before MDK reuses upload buffers or C# reclaims old textures.
    return state->waitForFences(state->device, 1, &state->fence, VK_TRUE, UINT64_MAX) == VK_SUCCESS && rendered;
}

void MDKUnityVulkanRelease(MDKUnityPlayer& player, UnityGfxRenderer renderer)
{
    if (renderer != kUnityGfxRendererVulkan)
        return;
    delete static_cast<VulkanState*>(player.backendState);
    player.backendState = nullptr;
}

void MDKUnityVulkanDeviceEvent(UnityGfxDeviceEventType eventType, IUnityInterfaces* interfaces,
                               UnityGfxRenderer renderer)
{
    if ((eventType == kUnityGfxDeviceEventInitialize || eventType == kUnityGfxDeviceEventAfterReset)
        && interfaces && renderer == kUnityGfxRendererVulkan) {
        g_vk2 = interfaces->Get<IUnityGraphicsVulkanV2>();
        g_vk1 = interfaces->Get<IUnityGraphicsVulkan>();
        UnityVulkanPluginEventConfig config{
            .renderPassPrecondition = kUnityVulkanRenderPass_EnsureOutside,
            .graphicsQueueAccess = kUnityVulkanGraphicsQueueAccess_DontCare,
            .flags = 1u << 2 | 1u << 3,
        };
        if (g_vk2) {
            g_vk2->ConfigureEvent(1, &config);
            g_vk2->ConfigureEvent(2, &config);
        } else if (g_vk1) {
            g_vk1->ConfigureEvent(1, &config);
            g_vk1->ConfigureEvent(2, &config);
        }
    } else if (eventType == kUnityGfxDeviceEventShutdown || eventType == kUnityGfxDeviceEventBeforeReset) {
        g_vk2 = nullptr;
        g_vk1 = nullptr;
    }
}

#else

bool MDKUnityVulkanDispatchEvent(int, const std::shared_ptr<MDKUnityPlayer>&, UnityRenderingEventAndData) { return false; }
const UnityRenderTarget* MDKUnityVulkanEventTarget() { return nullptr; }
bool MDKUnityVulkanInitialize(MDKUnityPlayer&, UnityGfxRenderer) { return false; }
bool MDKUnityVulkanRender(MDKUnityPlayer&, UnityGfxRenderer) { return false; }
void MDKUnityVulkanRelease(MDKUnityPlayer&, UnityGfxRenderer) {}
void MDKUnityVulkanDeviceEvent(UnityGfxDeviceEventType, IUnityInterfaces*, UnityGfxRenderer) {}

#endif
