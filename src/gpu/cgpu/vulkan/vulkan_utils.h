#pragma once
#include "cgpu/api.h"
#include "cgpu/backend/vulkan/cgpu_vulkan.h"
#include "cgpu/extensions/cgpu_vulkan_exts.h"
#include "cgpu/backend/vulkan/vk_mem_alloc.h"
#include "../common/common_utils.h"
#include "cgpu/flags.h"

#ifdef __cplusplus
extern "C" {
#endif

struct VkUtil_DescriptorPool;

// Environment Setup
bool VkUtil_InitializeEnvironment(struct CGPUInstance* Inst);
void VkUtil_DeInitializeEnvironment(struct CGPUInstance* Inst);

// Instance Helpers
void VkUtil_EnableValidationLayer(
CGPUInstance_Vulkan* I,
const VkDebugUtilsMessengerCreateInfoEXT* messenger_info_ptr,
const VkDebugReportCallbackCreateInfoEXT* report_info_ptr);
void VkUtil_QueryAllAdapters(CGPUInstance_Vulkan* I,
const char* const* device_layers, uint32_t device_layers_count,
const char* const* device_extensions, uint32_t device_extension_count);

// Device Helpers
void VkUtil_CreatePipelineCache(CGPUDevice_Vulkan* D);
void VkUtil_CreateVMAAllocator(CGPUInstance_Vulkan* I, CGPUAdapter_Vulkan* A, CGPUDevice_Vulkan* D);
void VkUtil_FreeVMAAllocator(CGPUInstance_Vulkan* I, CGPUAdapter_Vulkan* A, CGPUDevice_Vulkan* D);
void VkUtil_FreePipelineCache(CGPUInstance_Vulkan* I, CGPUAdapter_Vulkan* A, CGPUDevice_Vulkan* D);

// API Objects Helpers
struct VkUtil_DescriptorPool* VkUtil_CreateDescriptorPool(CGPUDevice_Vulkan* D);
void VkUtil_ConsumeDescriptorSets(struct VkUtil_DescriptorPool* pPool, const VkDescriptorSetLayout* pLayouts, VkDescriptorSet* pSets, uint32_t numDescriptorSets);
void VkUtil_ReturnDescriptorSets(struct VkUtil_DescriptorPool* pPool, VkDescriptorSet* pSets, uint32_t numDescriptorSets);
void VkUtil_FreeDescriptorPool(struct VkUtil_DescriptorPool* DescPool);
VkDescriptorSetLayout VkUtil_CreateDescriptorSetLayout(CGPUDevice_Vulkan* D, const VkDescriptorSetLayoutBinding* bindings, uint32_t bindings_count);
void VkUtil_FreeDescriptorSetLayout(CGPUDevice_Vulkan* D, VkDescriptorSetLayout layout);
void VkUtil_InitializeShaderReflection(CGPUDeviceId device, CGPUShaderLibrary_Vulkan* library, const struct CGPUShaderLibraryDescriptor* desc);
void VkUtil_FreeShaderReflection(CGPUShaderLibrary_Vulkan* library);

// Feature Select Helpers
void VkUtil_SelectQueueIndices(CGPUAdapter_Vulkan* VkAdapter);
void VkUtil_RecordAdapterDetail(CGPUAdapter_Vulkan* VkAdapter);
void VkUtil_EnumFormatSupports(CGPUAdapter_Vulkan* VkAdapter);
void VkUtil_SelectInstanceLayers(struct CGPUInstance_Vulkan* VkInstance,
const char* const* instance_layers, uint32_t instance_layers_count);
void VkUtil_SelectInstanceExtensions(struct CGPUInstance_Vulkan* VkInstance,
const char* const* instance_extensions, uint32_t instance_extension_count);
void VkUtil_SelectPhysicalDeviceLayers(struct CGPUAdapter_Vulkan* VkAdapter,
const char* const* device_layers, uint32_t device_layers_count);
void VkUtil_SelectPhysicalDeviceExtensions(struct CGPUAdapter_Vulkan* VkAdapter,
const char* const* device_extensions, uint32_t device_extension_count);

// Table Helpers
struct VkUtil_RenderPassDesc;
struct VkUtil_FramebufferDesc;
VkRenderPass VkUtil_RenderPassTableTryFind(struct CGPUVkPassTable* table, const struct VkUtil_RenderPassDesc* desc);
void VkUtil_RenderPassTableAdd(struct CGPUVkPassTable* table, const struct VkUtil_RenderPassDesc* desc, VkRenderPass pass);
VkFramebuffer VkUtil_FramebufferTableTryFind(struct CGPUVkPassTable* table, const struct VkUtil_FramebufferDesc* desc);
void VkUtil_FramebufferTableAdd(struct CGPUVkPassTable* table, const struct VkUtil_FramebufferDesc* desc, VkFramebuffer framebuffer);

// Debug Helpers
VKAPI_ATTR VkBool32 VKAPI_CALL VkUtil_DebugUtilsCallback(
VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
VkDebugUtilsMessageTypeFlagsEXT messageType,
const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
void* pUserData);
VKAPI_ATTR VkBool32 VKAPI_CALL VkUtil_DebugReportCallback(
VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
uint64_t object, size_t location, int32_t messageCode,
const char* pLayerPrefix, const char* pMessage, void* pUserData);
void VkUtil_OptionalSetObjectName(struct CGPUDevice_Vulkan* device, uint64_t handle, VkObjectType type, const char* name);

#define CGPU_VK_DESCRIPTOR_TYPE_RANGE_SIZE (VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1)
static const VkDescriptorPoolSize gDescriptorPoolSizes[CGPU_VK_DESCRIPTOR_TYPE_RANGE_SIZE] = {
    { VK_DESCRIPTOR_TYPE_SAMPLER, 1024 },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 8192 },
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1024 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1024 },
    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1024 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8192 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1024 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1 },
    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1 },
};
typedef struct VkUtil_DescriptorPool {
    CGPUDevice_Vulkan* Device;
    VkDescriptorPool pVkDescPool;
    VkDescriptorPoolCreateFlags mFlags;
    /// Lock for multi-threaded descriptor allocations
    struct SMutex* pMutex;
} VkUtil_DescriptorPool;

typedef struct VkUtil_RenderPassDesc {
    ECGPUFormat pColorFormats[CGPU_MAX_MRT_COUNT];
    ECGPULoadAction pLoadActionsColor[CGPU_MAX_MRT_COUNT];
    ECGPUStoreAction pStoreActionsColor[CGPU_MAX_MRT_COUNT];
    ECGPULoadAction pLoadActionsColorResolve[CGPU_MAX_MRT_COUNT];
    ECGPUStoreAction pStoreActionsColorResolve[CGPU_MAX_MRT_COUNT];
    bool pResolveMasks[CGPU_MAX_MRT_COUNT];
    uint32_t mColorAttachmentCount;
    ECGPUSampleCount mSampleCount;
    ECGPUFormat mDepthStencilFormat;
    ECGPULoadAction mLoadActionDepth;
    ECGPUStoreAction mStoreActionDepth;
    ECGPULoadAction mLoadActionStencil;
    ECGPUStoreAction mStoreActionStencil;
} VkUtil_RenderPassDesc;

typedef struct VkUtil_FramebufferDesc {
    VkRenderPass pRenderPass;
    uint32_t mAttachmentCount;
    VkImageView pImageViews[CGPU_MAX_MRT_COUNT * 2 + 1];
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mLayers;
} VkUtil_FramebufferDesc;

#define CHECK_VKRESULT(exp)                                                             \
    {                                                                                   \
        VkResult vkres = (exp);                                                         \
        if (VK_SUCCESS != vkres)                                                        \
        {                                                                               \
            cgpu_error("VKRESULT %s: FAILED with VkResult: %u", #exp, (uint32_t)vkres); \
            cgpu_assert(0);                                                             \
        }                                                                               \
    }

static const char* validation_layer_name = "VK_LAYER_KHRONOS_validation";
static const char* cgpu_wanted_instance_exts[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(_MACOS)
    VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_GGP)
    VK_GGP_STREAM_DESCRIPTOR_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_VI_NN)
    VK_NN_VI_SURFACE_EXTENSION_NAME,
#endif
    VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
    // To legally use HDR formats
    VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,
    /************************************************************************/
    // VR Extensions
    /************************************************************************/
    VK_KHR_DISPLAY_EXTENSION_NAME,
    VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME,
/************************************************************************/
// Multi GPU Extensions
/************************************************************************/
#if VK_KHR_device_group_creation
    VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,
#endif
#ifndef NX64
    /************************************************************************/
    // Property querying extensions
    /************************************************************************/
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
#endif
};

static const char* cgpu_wanted_device_exts[] = {
    "VK_KHR_portability_subset",
    VK_GOOGLE_USER_TYPE_EXTENSION_NAME,
    VK_GOOGLE_HLSL_FUNCTIONALITY_1_EXTENSION_NAME,
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_MAINTENANCE1_EXTENSION_NAME,
    VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
    VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME,
    VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME,
    VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
#ifdef USE_EXTERNAL_MEMORY_EXTENSIONS
    VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
    VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
    VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME,
    #if defined(VK_USE_PLATFORM_WIN32_KHR)
    VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
    VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
    VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME,
    #endif
#endif
// Debug marker extension in case debug utils is not supported
#ifndef ENABLE_DEBUG_UTILS_EXTENSION
    VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
#endif
#if defined(VK_USE_PLATFORM_GGP)
    VK_GGP_FRAME_TOKEN_EXTENSION_NAME,
#endif

#if VK_KHR_draw_indirect_count
    VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
#endif
// Fragment shader interlock extension to be used for ROV type functionality in Vulkan
#if VK_EXT_fragment_shader_interlock
    VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME,
#endif
/************************************************************************/
// NVIDIA Specific Extensions
/************************************************************************/
#ifdef USE_NV_EXTENSIONS
    VK_NVX_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME,
#endif
    /************************************************************************/
    // AMD Specific Extensions
    /************************************************************************/
    VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
    VK_AMD_SHADER_BALLOT_EXTENSION_NAME,
    VK_AMD_GCN_SHADER_EXTENSION_NAME,
/************************************************************************/
// Multi GPU Extensions
/************************************************************************/
#if VK_KHR_device_group
    VK_KHR_DEVICE_GROUP_EXTENSION_NAME,
#endif
    /************************************************************************/
    // Bindless & None Uniform access Extensions
    /************************************************************************/
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
#if VK_KHR_maintenance3 // descriptor indexing depends on this
    VK_KHR_MAINTENANCE3_EXTENSION_NAME,
#endif
    /************************************************************************/
    // Descriptor Update Template Extension for efficient descriptor set updates
    /************************************************************************/
    VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,
/************************************************************************/
// Raytracing
/************************************************************************/
#ifdef ENABLE_RAYTRACING
    VK_NV_RAY_TRACING_EXTENSION_NAME,
#endif
/************************************************************************/
// YCbCr format support
/************************************************************************/
#if VK_KHR_bind_memory2
    // Requirement for VK_KHR_sampler_ycbcr_conversion
    VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
#endif
#if VK_KHR_sampler_ycbcr_conversion
    VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
#endif
/************************************************************************/
// Nsight Aftermath
/************************************************************************/
#ifdef ENABLE_NSIGHT_AFTERMATH
    VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME,
    VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,
#endif
    /************************************************************************/
    /************************************************************************/
};

#ifdef __cplusplus
}
#endif

#include "vulkan_utils.inl"