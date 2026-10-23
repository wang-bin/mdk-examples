// Unity Native Plugin API copyright © 2015-2026 Unity Technologies ApS
// AI participated
// Licensed under the Unity Companion License for Unity-dependent projects:
// https://unity3d.com/legal/licenses/Unity_Companion_License.
// Unless expressly provided otherwise, the Software under this license is made
// available strictly on an AS IS BASIS WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED. Please review the license for details on these and other conditions.
#pragma once
#include "IUnityInterface.h"
#include <vulkan/vulkan.h>

typedef struct RenderSurfaceBase* UnityRenderBuffer;
typedef unsigned int UnityTextureID;
struct UnityVulkanInstance {
    VkPipelineCache pipelineCache;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    PFN_vkGetInstanceProcAddr getInstanceProcAddr;
    unsigned int queueFamilyIndex;
    void* reserved[8];
};
struct UnityVulkanMemory {
    VkDeviceMemory memory;
    VkDeviceSize offset;
    VkDeviceSize size;
    void* mapped;
    VkMemoryPropertyFlags flags;
    unsigned int memoryTypeIndex;
    void* reserved[4];
};
struct UnityVulkanImage {
    UnityVulkanMemory memory;
    VkImage image;
    VkImageLayout layout;
    VkImageAspectFlags aspect;
    VkImageUsageFlags usage;
    VkFormat format;
    VkExtent3D extent;
    VkImageTiling tiling;
    VkImageType type;
    VkSampleCountFlagBits samples;
    int layers;
    int mipCount;
    void* reserved[4];
};
struct UnityVulkanRecordingState {
    VkCommandBuffer commandBuffer;
    VkCommandBufferLevel commandBufferLevel;
    VkRenderPass renderPass;
    VkFramebuffer framebuffer;
    int subPassIndex;
    unsigned long long currentFrameNumber;
    unsigned long long safeFrameNumber;
    void* reserved[4];
};
struct UnityVulkanBuffer {
    UnityVulkanMemory memory;
    VkBuffer buffer;
    size_t sizeInBytes;
    VkBufferUsageFlags usage;
    void* reserved[4];
};
enum UnityVulkanResourceAccessMode { kUnityVulkanResourceAccess_ObserveOnly, kUnityVulkanResourceAccess_PipelineBarrier, kUnityVulkanResourceAccess_Recreate };
enum UnityVulkanGraphicsQueueAccess { kUnityVulkanGraphicsQueueAccess_DontCare, kUnityVulkanGraphicsQueueAccess_Allow };
enum UnityVulkanRenderPassPrecondition { kUnityVulkanRenderPass_DontCare, kUnityVulkanRenderPass_EnsureInside, kUnityVulkanRenderPass_EnsureOutside };
struct UnityVulkanPluginEventConfig {
    UnityVulkanRenderPassPrecondition renderPassPrecondition;
    UnityVulkanGraphicsQueueAccess graphicsQueueAccess;
    unsigned int flags;
};
static const VkImageSubresource* const UnityVulkanWholeImage = nullptr;
typedef PFN_vkGetInstanceProcAddr (UNITY_INTERFACE_API *UnityVulkanInitCallback)(PFN_vkGetInstanceProcAddr, void*);

UNITY_DECLARE_INTERFACE(IUnityGraphicsVulkanV2) {
    bool (UNITY_INTERFACE_API *InterceptInitialization)(UnityVulkanInitCallback, void*);
    PFN_vkVoidFunction (UNITY_INTERFACE_API *InterceptVulkanAPI)(const char*, PFN_vkVoidFunction);
    void (UNITY_INTERFACE_API *ConfigureEvent)(int, const UnityVulkanPluginEventConfig*);
    UnityVulkanInstance (UNITY_INTERFACE_API *Instance)();
    bool (UNITY_INTERFACE_API *CommandRecordingState)(UnityVulkanRecordingState*, UnityVulkanGraphicsQueueAccess);
    bool (UNITY_INTERFACE_API *AccessTexture)(void*, const VkImageSubresource*, VkImageLayout, VkPipelineStageFlags, VkAccessFlags, UnityVulkanResourceAccessMode, UnityVulkanImage*);
    bool (UNITY_INTERFACE_API *AccessRenderBufferTexture)(void*, const VkImageSubresource*, VkImageLayout, VkPipelineStageFlags, VkAccessFlags, UnityVulkanResourceAccessMode, UnityVulkanImage*);
    bool (UNITY_INTERFACE_API *AccessRenderBufferResolveTexture)(UnityRenderBuffer, const VkImageSubresource*, VkImageLayout, VkPipelineStageFlags, VkAccessFlags, UnityVulkanResourceAccessMode, UnityVulkanImage*);
    bool (UNITY_INTERFACE_API *AccessBuffer)(void*, VkPipelineStageFlags, VkAccessFlags, UnityVulkanResourceAccessMode, UnityVulkanBuffer*);
    void (UNITY_INTERFACE_API *EnsureOutsideRenderPass)();
    void (UNITY_INTERFACE_API *EnsureInsideRenderPass)();
    void (UNITY_INTERFACE_API *AccessQueue)(UnityRenderingEventAndData, int, void*, bool);
};
UNITY_REGISTER_INTERFACE_GUID(0x329334C09DCA4787ULL, 0xB347DD92A0097FFCULL, IUnityGraphicsVulkanV2)

UNITY_DECLARE_INTERFACE(IUnityGraphicsVulkan) {
    bool (UNITY_INTERFACE_API *InterceptInitialization)(UnityVulkanInitCallback, void*);
    PFN_vkVoidFunction (UNITY_INTERFACE_API *InterceptVulkanAPI)(const char*, PFN_vkVoidFunction);
    void (UNITY_INTERFACE_API *ConfigureEvent)(int, const UnityVulkanPluginEventConfig*);
    UnityVulkanInstance (UNITY_INTERFACE_API *Instance)();
    bool (UNITY_INTERFACE_API *CommandRecordingState)(UnityVulkanRecordingState*, UnityVulkanGraphicsQueueAccess);
    bool (UNITY_INTERFACE_API *AccessTexture)(void*, const VkImageSubresource*, VkImageLayout, VkPipelineStageFlags, VkAccessFlags, UnityVulkanResourceAccessMode, UnityVulkanImage*);
    bool (UNITY_INTERFACE_API *AccessRenderBufferTexture)(void*, const VkImageSubresource*, VkImageLayout, VkPipelineStageFlags, VkAccessFlags, UnityVulkanResourceAccessMode, UnityVulkanImage*);
    bool (UNITY_INTERFACE_API *AccessRenderBufferResolveTexture)(UnityRenderBuffer, const VkImageSubresource*, VkImageLayout, VkPipelineStageFlags, VkAccessFlags, UnityVulkanResourceAccessMode, UnityVulkanImage*);
    bool (UNITY_INTERFACE_API *AccessBuffer)(void*, VkPipelineStageFlags, VkAccessFlags, UnityVulkanResourceAccessMode, UnityVulkanBuffer*);
    void (UNITY_INTERFACE_API *EnsureOutsideRenderPass)();
    void (UNITY_INTERFACE_API *EnsureInsideRenderPass)();
    void (UNITY_INTERFACE_API *AccessQueue)(UnityRenderingEventAndData, int, void*, bool);
};
UNITY_REGISTER_INTERFACE_GUID(0x95355348D4EF4E11ULL, 0x9789313DFCFFCC87ULL, IUnityGraphicsVulkan)
