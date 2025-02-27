#include "vulkan_utils.h"
#include "cgpu/backend/vulkan/cgpu_vulkan.h"
#include "cgpu/drivers/cgpu_ags.h"
#include "cgpu/drivers/cgpu_nvapi.h"
#include "cgpu/flags.h"
#include "vulkan/vulkan_core.h"
#ifdef CGPU_THREAD_SAFETY
    #include "platform/thread.h"
#endif
#include "cgpu/shader-reflections/spirv/spirv_reflect.h"
#include <stdio.h>
#include <string.h>

bool VkUtil_InitializeEnvironment(struct CGPUInstance* Inst)
{
    // AGS
    bool AGS_started = false;
    AGS_started = (cgpu_ags_init(Inst) == CGPU_AGS_SUCCESS);
    (void)AGS_started;
    // NVAPI
    bool NVAPI_started = false;
    NVAPI_started = (cgpu_nvapi_init(Inst) == CGPU_NVAPI_OK);
    (void)NVAPI_started;
    // VOLK
#if !defined(NX64)
    VkResult volkInit = volkInitialize();
    if (volkInit != VK_SUCCESS)
    {
        cgpu_assert((volkInit == VK_SUCCESS) && "Volk Initialize Failed!");
        return false;
    }
#endif
    return true;
}

void VkUtil_DeInitializeEnvironment(struct CGPUInstance* Inst)
{
    cgpu_ags_exit();
    Inst->ags_status = CGPU_AGS_NONE;
    cgpu_nvapi_exit();
    Inst->nvapi_status = CGPU_NVAPI_NONE;
}

// Instance APIs
void VkUtil_EnableValidationLayer(
CGPUInstance_Vulkan* I,
const VkDebugUtilsMessengerCreateInfoEXT* messenger_info_ptr,
const VkDebugReportCallbackCreateInfoEXT* report_info_ptr)
{
    if (I->debug_utils)
    {
        VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pfnUserCallback = VkUtil_DebugUtilsCallback,
            .messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .flags = 0,
            .pUserData = NULL
        };
        const VkDebugUtilsMessengerCreateInfoEXT* messengerInfoPtr =
        (messenger_info_ptr != CGPU_NULLPTR) ? messenger_info_ptr : &messengerInfo;

        cgpu_assert(vkCreateDebugUtilsMessengerEXT && "Load vkCreateDebugUtilsMessengerEXT failed!");
        VkResult res = vkCreateDebugUtilsMessengerEXT(I->pVkInstance,
        messengerInfoPtr, GLOBAL_VkAllocationCallbacks,
        &(I->pVkDebugUtilsMessenger));
        if (VK_SUCCESS != res)
        {
            cgpu_assert(0 && "vkCreateDebugUtilsMessengerEXT failed - disabling Vulkan debug callbacks");
        }
    }
    else if (I->debug_report)
    {
        VkDebugReportCallbackCreateInfoEXT reportInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
            .pNext = NULL,
            .pfnCallback = VkUtil_DebugReportCallback,
            .flags =
#if defined(NX64) || defined(__ANDROID__)
            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | // Performance warnings are not very vaild on desktop
#endif
            VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT /* | VK_DEBUG_REPORT_INFORMATION_BIT_EXT*/
        };
        const VkDebugReportCallbackCreateInfoEXT* reportInfoPtr =
        (report_info_ptr != CGPU_NULLPTR) ? report_info_ptr : &reportInfo;
        VkResult res = vkCreateDebugReportCallbackEXT(I->pVkInstance,
        reportInfoPtr, GLOBAL_VkAllocationCallbacks,
        &(I->pVkDebugReport));
        cgpu_assert(vkCreateDebugUtilsMessengerEXT && "Load vkCreateDebugReportCallbackEXT failed!");
        if (VK_SUCCESS != res)
        {
            cgpu_assert(0 && "vkCreateDebugReportCallbackEXT failed - disabling Vulkan debug callbacks");
        }
    }
}
#ifdef __clang__
#endif
void VkUtil_QueryAllAdapters(CGPUInstance_Vulkan* I,
const char* const* device_layers, uint32_t device_layers_count,
const char* const* device_extensions, uint32_t device_extension_count)
{
    cgpu_assert((I->mPhysicalDeviceCount == 0) && "VkUtil_QueryAllAdapters should only be called once!");

    vkEnumeratePhysicalDevices(I->pVkInstance, &I->mPhysicalDeviceCount, CGPU_NULLPTR);
    if (I->mPhysicalDeviceCount != 0)
    {
        I->pVulkanAdapters =
        (CGPUAdapter_Vulkan*)cgpu_calloc(I->mPhysicalDeviceCount, sizeof(CGPUAdapter_Vulkan));
        DECLARE_ZERO_VLA(VkPhysicalDevice, pysicalDevices, I->mPhysicalDeviceCount)
        vkEnumeratePhysicalDevices(I->pVkInstance, &I->mPhysicalDeviceCount,
        pysicalDevices);
        for (uint32_t i = 0; i < I->mPhysicalDeviceCount; i++)
        {
            // Alloc & Zero Adapter
            CGPUAdapter_Vulkan* VkAdapter = &I->pVulkanAdapters[i];
            for (uint32_t q = 0; q < CGPU_QUEUE_TYPE_COUNT; q++)
            {
                VkAdapter->mQueueFamilyIndices[q] = -1;
            }
            VkAdapter->pPhysicalDevice = pysicalDevices[i];
            // Query Physical Device Properties
            VkAdapter->mSubgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
            VkAdapter->mSubgroupProperties.pNext = NULL;
            VkAdapter->mPhysicalDeviceProps.pNext = &VkAdapter->mSubgroupProperties;
            VkAdapter->mPhysicalDeviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
            vkGetPhysicalDeviceProperties2(pysicalDevices[i], &VkAdapter->mPhysicalDeviceProps);
            // Query Physical Device Features
            VkAdapter->mPhysicalDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
#ifndef NX64
            vkGetPhysicalDeviceFeatures2KHR(pysicalDevices[i], &VkAdapter->mPhysicalDeviceFeatures);
#else
            vkGetPhysicalDeviceFeatures2(pysicalDevices[i], &VkAdapter->mPhysicalDeviceFeatures);
#endif
            // Enumerate Format Supports
            VkUtil_EnumFormatSupports(VkAdapter);
            // Query Physical Device Layers Properties
            VkUtil_SelectPhysicalDeviceLayers(VkAdapter, device_layers, device_layers_count);
            // Query Physical Device Extension Properties
            VkUtil_SelectPhysicalDeviceExtensions(VkAdapter, device_extensions, device_extension_count);
            // Select Queue Indices
            VkUtil_SelectQueueIndices(VkAdapter);
            // Record Adapter Detail
            VkUtil_RecordAdapterDetail(VkAdapter);
        }
    }
    else
    {
        cgpu_assert(0 && "Vulkan: 0 physical device avalable!");
    }
}

// Device APIs
void VkUtil_CreatePipelineCache(CGPUDevice_Vulkan* D)
{
    cgpu_assert((D->pPipelineCache == VK_NULL_HANDLE) && "VkUtil_CreatePipelineCache should be called only once!");

    // TODO: serde
    VkPipelineCacheCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext = NULL,
        .initialDataSize = 0,
        .pInitialData = NULL
    };
    D->mVkDeviceTable.vkCreatePipelineCache(D->pVkDevice,
    &info, GLOBAL_VkAllocationCallbacks, &D->pPipelineCache);
}

// Shader Reflection
static const ECGPUResourceType RTLut[] = {
    CGPU_RESOURCE_TYPE_SAMPLER,                // SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER
    CGPU_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER, // SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    CGPU_RESOURCE_TYPE_TEXTURE,                // SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE
    CGPU_RESOURCE_TYPE_RW_TEXTURE,             // SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE
    CGPU_RESOURCE_TYPE_TEXEL_BUFFER,           // SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
    CGPU_RESOURCE_TYPE_RW_TEXEL_BUFFER,        // SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
    CGPU_RESOURCE_TYPE_UNIFORM_BUFFER,         // SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    CGPU_RESOURCE_TYPE_RW_BUFFER,              // SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER
    CGPU_RESOURCE_TYPE_UNIFORM_BUFFER,         // SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
    CGPU_RESOURCE_TYPE_RW_BUFFER,              // SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
    CGPU_RESOURCE_TYPE_INPUT_ATTACHMENT,       // SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
    CGPU_RESOURCE_TYPE_RAY_TRACING             // SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
};
static ECGPUTextureDimension DIMLut[SpvDimSubpassData + 1] = {
    CGPU_TEX_DIMENSION_1D,        // SpvDim1D
    CGPU_TEX_DIMENSION_2D,        // SpvDim2D
    CGPU_TEX_DIMENSION_3D,        // SpvDim3D
    CGPU_TEX_DIMENSION_CUBE,      // SpvDimCube
    CGPU_TEX_DIMENSION_UNDEFINED, // SpvDimRect
    CGPU_TEX_DIMENSION_UNDEFINED, // SpvDimBuffer
    CGPU_TEX_DIMENSION_UNDEFINED  // SpvDimSubpassData
};
static ECGPUTextureDimension ArrDIMLut[SpvDimSubpassData + 1] = {
    CGPU_TEX_DIMENSION_1D_ARRAY,   // SpvDim1D
    CGPU_TEX_DIMENSION_2D_ARRAY,   // SpvDim2D
    CGPU_TEX_DIMENSION_UNDEFINED,  // SpvDim3D
    CGPU_TEX_DIMENSION_CUBE_ARRAY, // SpvDimCube
    CGPU_TEX_DIMENSION_UNDEFINED,  // SpvDimRect
    CGPU_TEX_DIMENSION_UNDEFINED,  // SpvDimBuffer
    CGPU_TEX_DIMENSION_UNDEFINED   // SpvDimSubpassData
};
const char8_t* push_constants_name = "push_constants";
void VkUtil_InitializeShaderReflection(CGPUDeviceId device, CGPUShaderLibrary_Vulkan* S, const struct CGPUShaderLibraryDescriptor* desc)
{
    S->pReflect = (SpvReflectShaderModule*)cgpu_calloc(1, sizeof(SpvReflectShaderModule));
    SpvReflectResult spvRes = spvReflectCreateShaderModule(desc->code_size, desc->code, S->pReflect);
    (void)spvRes;
    cgpu_assert(spvRes == SPV_REFLECT_RESULT_SUCCESS && "Failed to Reflect Shader!");
    uint32_t entry_count = S->pReflect->entry_point_count;
    S->super.entrys_count = entry_count;
    S->super.entry_reflections = cgpu_calloc(entry_count, sizeof(CGPUShaderReflection));
    for (uint32_t i = 0; i < entry_count; i++)
    {
        // Initialize Common Reflection Data
        CGPUShaderReflection* reflection = &S->super.entry_reflections[i];
        // ATTENTION: We have only one entry point now
        const SpvReflectEntryPoint* entry = spvReflectGetEntryPoint(S->pReflect, S->pReflect->entry_points[i].name);
        reflection->entry_name = (const char8_t*)entry->name;
        reflection->stage = (ECGPUShaderStage)entry->shader_stage;
        if (reflection->stage == CGPU_SHADER_STAGE_COMPUTE)
        {
            reflection->thread_group_sizes[0] = entry->local_size.x;
            reflection->thread_group_sizes[1] = entry->local_size.y;
            reflection->thread_group_sizes[2] = entry->local_size.z;
        }
        const bool bGLSL = S->pReflect->source_language & SpvSourceLanguageGLSL;
        (void)bGLSL;
        const bool bHLSL = S->pReflect->source_language & SpvSourceLanguageHLSL;
        uint32_t icount;
        spvReflectEnumerateInputVariables(S->pReflect, &icount, NULL);
        if (icount > 0)
        {
            DECLARE_ZERO_VLA(SpvReflectInterfaceVariable*, input_vars, icount)
            spvReflectEnumerateInputVariables(S->pReflect, &icount, input_vars);
            if ((entry->shader_stage & SPV_REFLECT_SHADER_STAGE_VERTEX_BIT))
            {
                reflection->vertex_inputs_count = icount;
                reflection->vertex_inputs = cgpu_calloc(icount, sizeof(CGPUVertexInput));
                // Handle Vertex Inputs
                for (uint32_t i = 0; i < icount; i++)
                {
                    // We use semantic for HLSL sources because DXC is a piece of shit.
                    reflection->vertex_inputs[i].name =
                    bHLSL ? input_vars[i]->semantic : input_vars[i]->name;
                    reflection->vertex_inputs[i].format =
                    VkUtil_FormatTranslateToCGPU((VkFormat)input_vars[i]->format);
                }
            }
        }
        // Handle Descriptor Sets
        uint32_t scount;
        uint32_t ccount;
        spvReflectEnumeratePushConstantBlocks(S->pReflect, &ccount, NULL);
        spvReflectEnumerateDescriptorSets(S->pReflect, &scount, NULL);
        if (scount > 0 || ccount > 0)
        {
            DECLARE_ZERO_VLA(SpvReflectDescriptorSet*, descriptros_sets, scount + 1)
            DECLARE_ZERO_VLA(SpvReflectBlockVariable*, root_sets, ccount + 1)
            spvReflectEnumerateDescriptorSets(S->pReflect, &scount, descriptros_sets);
            spvReflectEnumeratePushConstantBlocks(S->pReflect, &ccount, root_sets);
            uint32_t bcount = 0;
            for (uint32_t i = 0; i < scount; i++)
            {
                bcount += descriptros_sets[i]->binding_count;
            }
            bcount += ccount;
            reflection->shader_resources_count = bcount;
            reflection->shader_resources = cgpu_calloc(bcount, sizeof(CGPUShaderResource));
            // Fill Shader Resources
            uint32_t i_res = 0;
            for (uint32_t i_set = 0; i_set < scount; i_set++)
            {
                SpvReflectDescriptorSet* current_set = descriptros_sets[i_set];
                for (uint32_t i_binding = 0; i_binding < current_set->binding_count; i_binding++, i_res++)
                {
                    SpvReflectDescriptorBinding* current_binding = current_set->bindings[i_binding];
                    CGPUShaderResource* current_res = &reflection->shader_resources[i_res];
                    current_res->set = current_binding->set;
                    current_res->binding = current_binding->binding;
                    current_res->stages = S->pReflect->shader_stage;
                    current_res->type = RTLut[current_binding->descriptor_type];
                    current_res->name = current_binding->name;
                    current_res->name_hash =
                    cgpu_hash(current_binding->name, strlen(current_binding->name), (size_t)device);
                    current_res->size = current_binding->count;
                    // Solve Dimension
                    if ((current_binding->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_EXTERNAL_IMAGE) ||
                        (current_binding->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLED_IMAGE))
                    {
                        if (current_binding->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY)
                            current_res->dim = ArrDIMLut[current_binding->image.dim];
                        else
                            current_res->dim = DIMLut[current_binding->image.dim];
                        if (current_binding->image.ms)
                        {
                            current_res->dim = current_res->dim & CGPU_TEX_DIMENSION_2D ? CGPU_TEX_DIMENSION_2DMS : current_res->dim;
                            current_res->dim = current_res->dim & CGPU_TEX_DIMENSION_2D_ARRAY ? CGPU_TEX_DIMENSION_2DMS_ARRAY : current_res->dim;
                        }
                    }
                }
            }
            // Fill Push Constants
            for (uint32_t i = 0; i < ccount; i++)
            {
                CGPUShaderResource* current_res = &reflection->shader_resources[i_res + i];
                current_res->set = 0;
                current_res->type = CGPU_RESOURCE_TYPE_PUSH_CONSTANT;
                current_res->binding = 0;
                current_res->name = push_constants_name;
                current_res->name_hash =
                cgpu_hash(current_res->name, strlen(current_res->name), (size_t)device);
                current_res->stages = S->pReflect->shader_stage;
                current_res->size = root_sets[i]->size;
                current_res->offset = root_sets[i]->offset;
            }
        }
    }
}

void VkUtil_FreeShaderReflection(CGPUShaderLibrary_Vulkan* S)
{
    spvReflectDestroyShaderModule(S->pReflect);
    if (S->super.entry_reflections)
    {
        for (uint32_t i = 0; i < S->super.entrys_count; i++)
        {
            CGPUShaderReflection* reflection = S->super.entry_reflections + i;
            if (reflection->vertex_inputs) cgpu_free(reflection->vertex_inputs);
            if (reflection->shader_resources) cgpu_free(reflection->shader_resources);
        }
    }
    cgpu_free(S->super.entry_reflections);
    cgpu_free(S->pReflect);
}

// VMA
void VkUtil_CreateVMAAllocator(CGPUInstance_Vulkan* I, CGPUAdapter_Vulkan* A,
CGPUDevice_Vulkan* D)
{
    VmaVulkanFunctions vulkanFunctions = {
        .vkAllocateMemory = D->mVkDeviceTable.vkAllocateMemory,
        .vkBindBufferMemory = D->mVkDeviceTable.vkBindBufferMemory,
        .vkBindImageMemory = D->mVkDeviceTable.vkBindImageMemory,
        .vkCreateBuffer = D->mVkDeviceTable.vkCreateBuffer,
        .vkCreateImage = D->mVkDeviceTable.vkCreateImage,
        .vkDestroyBuffer = D->mVkDeviceTable.vkDestroyBuffer,
        .vkDestroyImage = D->mVkDeviceTable.vkDestroyImage,
        .vkFreeMemory = D->mVkDeviceTable.vkFreeMemory,
        .vkGetBufferMemoryRequirements = D->mVkDeviceTable.vkGetBufferMemoryRequirements,
        .vkGetBufferMemoryRequirements2KHR = D->mVkDeviceTable.vkGetBufferMemoryRequirements2KHR,
        .vkGetImageMemoryRequirements = D->mVkDeviceTable.vkGetImageMemoryRequirements,
        .vkGetImageMemoryRequirements2KHR = D->mVkDeviceTable.vkGetImageMemoryRequirements2KHR,
        .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
        .vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
        .vkMapMemory = D->mVkDeviceTable.vkMapMemory,
        .vkUnmapMemory = D->mVkDeviceTable.vkUnmapMemory,
        .vkFlushMappedMemoryRanges = D->mVkDeviceTable.vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges = D->mVkDeviceTable.vkInvalidateMappedMemoryRanges,
        .vkCmdCopyBuffer = D->mVkDeviceTable.vkCmdCopyBuffer
    };
    VmaAllocatorCreateInfo vmaInfo = {
        .device = D->pVkDevice,
        .physicalDevice = A->pPhysicalDevice,
        .instance = I->pVkInstance,
        .pVulkanFunctions = &vulkanFunctions,
        .pAllocationCallbacks = GLOBAL_VkAllocationCallbacks
    };
    if (A->dedicated_allocation)
    {
        vmaInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
    }
    if (vmaCreateAllocator(&vmaInfo, &D->pVmaAllocator) != VK_SUCCESS)
    {
        cgpu_assert(0);
    }
}

void VkUtil_FreeVMAAllocator(CGPUInstance_Vulkan* I, CGPUAdapter_Vulkan* A, CGPUDevice_Vulkan* D)
{
    vmaDestroyAllocator(D->pVmaAllocator);
}

void VkUtil_FreePipelineCache(CGPUInstance_Vulkan* I, CGPUAdapter_Vulkan* A, CGPUDevice_Vulkan* D)
{
    if (D->pPipelineCache != VK_NULL_HANDLE)
    {
        D->mVkDeviceTable.vkDestroyPipelineCache(
        D->pVkDevice, D->pPipelineCache, GLOBAL_VkAllocationCallbacks);
    }
}

// API Objects Helpers
struct VkUtil_DescriptorPool* VkUtil_CreateDescriptorPool(CGPUDevice_Vulkan* D)
{
    VkUtil_DescriptorPool* Pool = (VkUtil_DescriptorPool*)cgpu_calloc(1, sizeof(VkUtil_DescriptorPool));
#ifdef CGPU_THREAD_SAFETY
    Pool->pMutex = (SMutex*)cgpu_calloc(1, sizeof(SMutex));
    skr_init_mutex(Pool->pMutex);
#endif
    VkDescriptorPoolCreateFlags flags = (VkDescriptorPoolCreateFlags)0;
    // TODO: It is possible to avoid using that flag by updating descriptor sets instead of deleting them.
    flags |= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    Pool->Device = D;
    Pool->mFlags = flags;
    VkDescriptorPoolCreateInfo poolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .poolSizeCount = CGPU_VK_DESCRIPTOR_TYPE_RANGE_SIZE,
        .pPoolSizes = gDescriptorPoolSizes,
        .flags = Pool->mFlags,
        .maxSets = 8192
    };
    CHECK_VKRESULT(D->mVkDeviceTable.vkCreateDescriptorPool(
    D->pVkDevice, &poolCreateInfo, GLOBAL_VkAllocationCallbacks, &Pool->pVkDescPool));
    return Pool;
}

void VkUtil_ConsumeDescriptorSets(struct VkUtil_DescriptorPool* pPool,
const VkDescriptorSetLayout* pLayouts, VkDescriptorSet* pSets, uint32_t numDescriptorSets)
{
#ifdef CGPU_THREAD_SAFETY
    skr_acquire_mutex(pPool->pMutex);
#endif
    {
        CGPUDevice_Vulkan* D = (CGPUDevice_Vulkan*)pPool->Device;
        VkDescriptorSetAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = NULL,
            .descriptorPool = pPool->pVkDescPool,
            .descriptorSetCount = numDescriptorSets,
            .pSetLayouts = pLayouts
        };
        VkResult vk_res = D->mVkDeviceTable.vkAllocateDescriptorSets(D->pVkDevice, &alloc_info, pSets);
        if (vk_res != VK_SUCCESS)
        {
            cgpu_assert(0 && "Descriptor Set used out, vk descriptor pool expansion not implemented!");
        }
    }
#ifdef CGPU_THREAD_SAFETY
    skr_release_mutex(pPool->pMutex);
#endif
}

void VkUtil_ReturnDescriptorSets(struct VkUtil_DescriptorPool* pPool, VkDescriptorSet* pSets, uint32_t numDescriptorSets)
{
#ifdef CGPU_THREAD_SAFETY
    skr_acquire_mutex(pPool->pMutex);
#endif
    {
        // TODO: It is possible to avoid using that flag by updating descriptor sets instead of deleting them.
        // The application can keep track of recycled descriptor sets and re-use one of them when a new one is requested.
        // Reference: https://arm-software.github.io/vulkan_best_practice_for_mobile_developers/samples/performance/descriptor_management/descriptor_management_tutorial.html
        CGPUDevice_Vulkan* D = (CGPUDevice_Vulkan*)pPool->Device;
        D->mVkDeviceTable.vkFreeDescriptorSets(D->pVkDevice, pPool->pVkDescPool, numDescriptorSets, pSets);
    }
#ifdef CGPU_THREAD_SAFETY
    skr_release_mutex(pPool->pMutex);
#endif
}

void VkUtil_FreeDescriptorPool(struct VkUtil_DescriptorPool* DescPool)
{
    CGPUDevice_Vulkan* D = DescPool->Device;
    D->mVkDeviceTable.vkDestroyDescriptorPool(D->pVkDevice, DescPool->pVkDescPool, GLOBAL_VkAllocationCallbacks);
#ifdef CGPU_THREAD_SAFETY
    if (DescPool->pMutex)
    {
        skr_destroy_mutex(DescPool->pMutex);
        cgpu_free(DescPool->pMutex);
    }
#endif
    cgpu_free(DescPool);
}

VkDescriptorSetLayout VkUtil_CreateDescriptorSetLayout(CGPUDevice_Vulkan* D,
const VkDescriptorSetLayoutBinding* bindings, uint32_t bindings_count)
{
    VkDescriptorSetLayout out_layout = VK_NULL_HANDLE;
    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .bindingCount = bindings_count,
        .pBindings = bindings,
        .flags = 0
    };
    CHECK_VKRESULT(D->mVkDeviceTable.vkCreateDescriptorSetLayout(
    D->pVkDevice, &layout_info, GLOBAL_VkAllocationCallbacks, &out_layout));
    return out_layout;
}

void VkUtil_FreeDescriptorSetLayout(CGPUDevice_Vulkan* D, VkDescriptorSetLayout layout)
{
    D->mVkDeviceTable.vkDestroyDescriptorSetLayout(D->pVkDevice, layout, GLOBAL_VkAllocationCallbacks);
}

// Select Helpers
void VkUtil_QueryHostVisbleVramInfo(CGPUAdapter_Vulkan* VkAdapter)
{
    CGPUAdapterDetail* adapter_detail = &VkAdapter->adapter_detail;
    adapter_detail->support_host_visible_vram = false;
#ifdef VK_EXT_memory_budget
    #if VK_EXT_memory_budget
    if (vkGetPhysicalDeviceMemoryProperties2KHR)
    {
        DECLARE_ZERO(VkPhysicalDeviceMemoryBudgetPropertiesEXT, budget)
        budget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
        DECLARE_ZERO(VkPhysicalDeviceMemoryProperties2, mem_prop2)
        mem_prop2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
        mem_prop2.pNext = &budget;
        vkGetPhysicalDeviceMemoryProperties2KHR(VkAdapter->pPhysicalDevice, &mem_prop2);
        VkPhysicalDeviceMemoryProperties mem_prop = mem_prop2.memoryProperties;
        for (uint32_t j = 0; j < mem_prop.memoryTypeCount; j++)
        {
            const uint32_t heap_index = mem_prop.memoryTypes[j].heapIndex;
            if (mem_prop.memoryHeaps[heap_index].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                const bool isDeviceLocal =
                mem_prop.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                const bool isHostVisible =
                mem_prop.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                if (isDeviceLocal && isHostVisible)
                {
                    adapter_detail->support_host_visible_vram = true;
                    adapter_detail->host_visible_vram_budget =
                    budget.heapBudget[heap_index] ?
                    budget.heapBudget[heap_index] :
                    mem_prop.memoryHeaps[heap_index].size;
                    break;
                }
            }
        }
    }
    else
    #endif
#endif
    {
        DECLARE_ZERO(VkPhysicalDeviceMemoryProperties, mem_prop)
        vkGetPhysicalDeviceMemoryProperties(VkAdapter->pPhysicalDevice, &mem_prop);
        for (uint32_t j = 0; j < mem_prop.memoryTypeCount; j++)
        {
            const uint32_t heap_index = mem_prop.memoryTypes[j].heapIndex;
            if (mem_prop.memoryHeaps[heap_index].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                const bool isDeviceLocal =
                mem_prop.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                const bool isHostVisible =
                mem_prop.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                if (isDeviceLocal && isHostVisible)
                {
                    adapter_detail->support_host_visible_vram = true;
                    adapter_detail->host_visible_vram_budget = mem_prop.memoryHeaps[heap_index].size;
                    break;
                }
                break;
            }
        }
    }
}

void VkUtil_RecordAdapterDetail(CGPUAdapter_Vulkan* VkAdapter)
{
    CGPUAdapterDetail* adapter_detail = &VkAdapter->adapter_detail;
    VkPhysicalDeviceProperties* prop = &VkAdapter->mPhysicalDeviceProps.properties;
    adapter_detail->is_cpu = prop->deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
    adapter_detail->is_virtual = prop->deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;
    adapter_detail->is_uma = prop->deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    cgpu_assert(prop->deviceType != VK_PHYSICAL_DEVICE_TYPE_OTHER && "VK_PHYSICAL_DEVICE_TYPE_OTHER not supported!");

    // vendor info
    adapter_detail->vendor_preset.device_id = prop->deviceID;
    adapter_detail->vendor_preset.vendor_id = prop->vendorID;
    adapter_detail->vendor_preset.driver_version = prop->driverVersion;
    const char* device_name = prop->deviceName;
    memcpy(adapter_detail->vendor_preset.gpu_name, device_name, strlen(device_name));

    // some features
    adapter_detail->uniform_buffer_alignment =
    (uint32_t)prop->limits.minUniformBufferOffsetAlignment;
    adapter_detail->upload_buffer_texture_alignment =
    (uint32_t)prop->limits.optimalBufferCopyOffsetAlignment;
    adapter_detail->upload_buffer_texture_row_alignment =
    (uint32_t)prop->limits.optimalBufferCopyRowPitchAlignment;
    adapter_detail->max_vertex_input_bindings = prop->limits.maxVertexInputBindings;
    adapter_detail->multidraw_indirect = prop->limits.maxDrawIndirectCount > 1;
    adapter_detail->wave_lane_count = VkAdapter->mSubgroupProperties.subgroupSize;
    adapter_detail->support_geom_shader = VkAdapter->mPhysicalDeviceFeatures.features.geometryShader;
    adapter_detail->support_tessellation = VkAdapter->mPhysicalDeviceFeatures.features.tessellationShader;
    // memory features
    VkUtil_QueryHostVisbleVramInfo(VkAdapter);
}

void VkUtil_SelectQueueIndices(CGPUAdapter_Vulkan* VkAdapter)
{
    // Query Queue Information.
    vkGetPhysicalDeviceQueueFamilyProperties(
    VkAdapter->pPhysicalDevice, &VkAdapter->mQueueFamiliesCount,
    CGPU_NULLPTR);
    VkAdapter->pQueueFamilyProperties = (VkQueueFamilyProperties*)cgpu_calloc(
    VkAdapter->mQueueFamiliesCount, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(VkAdapter->pPhysicalDevice,
    &VkAdapter->mQueueFamiliesCount, VkAdapter->pQueueFamilyProperties);

    for (uint32_t j = 0; j < VkAdapter->mQueueFamiliesCount; j++)
    {
        const VkQueueFamilyProperties* prop = &VkAdapter->pQueueFamilyProperties[j];
        if ((VkAdapter->mQueueFamilyIndices[CGPU_QUEUE_TYPE_GRAPHICS] == -1) &&
            (prop->queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            VkAdapter->mQueueFamilyIndices[CGPU_QUEUE_TYPE_GRAPHICS] = j;
        }
        else if ((VkAdapter->mQueueFamilyIndices[CGPU_QUEUE_TYPE_COMPUTE] == -1) &&
                 (prop->queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            VkAdapter->mQueueFamilyIndices[CGPU_QUEUE_TYPE_COMPUTE] = j;
        }
        else if ((VkAdapter->mQueueFamilyIndices[CGPU_QUEUE_TYPE_TRANSFER] == -1) &&
                 (prop->queueFlags & VK_QUEUE_TRANSFER_BIT))
        {
            VkAdapter->mQueueFamilyIndices[CGPU_QUEUE_TYPE_TRANSFER] = j;
        }
    }
}

void VkUtil_EnumFormatSupports(CGPUAdapter_Vulkan* VkAdapter)
{
    CGPUAdapterDetail* adapter_detail = (CGPUAdapterDetail*)&VkAdapter->adapter_detail;
    for (uint32_t i = 0; i < CGPU_FORMAT_COUNT; ++i)
    {
        VkFormatProperties formatSupport;
        adapter_detail->format_supports[i].shader_read = 0;
        adapter_detail->format_supports[i].shader_write = 0;
        adapter_detail->format_supports[i].render_target_write = 0;
        VkFormat fmt = (VkFormat)VkUtil_FormatTranslateToVk((ECGPUFormat)i);
        if (fmt == VK_FORMAT_UNDEFINED) continue;

        vkGetPhysicalDeviceFormatProperties(VkAdapter->pPhysicalDevice, fmt, &formatSupport);
        adapter_detail->format_supports[i].shader_read =
        (formatSupport.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
        adapter_detail->format_supports[i].shader_write =
        (formatSupport.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) != 0;
        adapter_detail->format_supports[i].render_target_write =
        (formatSupport.optimalTilingFeatures &
         (VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) != 0;
    }
    return;
}

void VkUtil_SelectInstanceLayers(struct CGPUInstance_Vulkan* vkInstance,
const char* const* instance_layers, uint32_t instance_layers_count)
{
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, NULL);
    if (count != 0)
    {
        vkInstance->pLayerNames = cgpu_calloc(instance_layers_count, sizeof(const char*));
        vkInstance->pLayerProperties = cgpu_calloc(instance_layers_count, sizeof(VkLayerProperties));

        DECLARE_ZERO_VLA(VkLayerProperties, layer_props, count)
        vkEnumerateInstanceLayerProperties(&count, layer_props);
        uint32_t filled_exts = 0;
        for (uint32_t i = 0; i < count; i++)
        {
            for (uint32_t j = 0; j < instance_layers_count; j++)
            {
                if (strcmp(layer_props[i].layerName, instance_layers[j]) == 0)
                {
                    vkInstance->pLayerProperties[filled_exts] = layer_props[i];
                    vkInstance->pLayerNames[filled_exts] = vkInstance->pLayerProperties[filled_exts].layerName;
                    filled_exts++;
                    break;
                }
            }
        }
        vkInstance->mLayersCount = filled_exts;
    }
    return;
}

void VkUtil_SelectInstanceExtensions(struct CGPUInstance_Vulkan* VkInstance,
const char* const* instance_extensions, uint32_t instance_extension_count)
{
    const char* layer_name = NULL; // Query Vulkan implementation or by implicitly enabled layers
    uint32_t count = 0;
    vkEnumerateInstanceExtensionProperties(layer_name, &count, NULL);
    if (count > 0)
    {
        VkInstance->pExtensionProperties = cgpu_calloc(instance_extension_count, sizeof(VkExtensionProperties));
        VkInstance->pExtensionNames = cgpu_calloc(instance_extension_count, sizeof(const char*));

        DECLARE_ZERO_VLA(VkExtensionProperties, ext_props, count)
        vkEnumerateInstanceExtensionProperties(layer_name, &count, ext_props);
        uint32_t filled_exts = 0;
        for (uint32_t i = 0; i < count; i++)
        {
            for (uint32_t j = 0; j < instance_extension_count; j++)
            {
                if (strcmp(ext_props[i].extensionName, instance_extensions[j]) == 0)
                {
                    VkInstance->pExtensionProperties[filled_exts] = ext_props[i];
                    VkInstance->pExtensionNames[filled_exts] = VkInstance->pExtensionProperties[filled_exts].extensionName;
                    filled_exts++;
                    break;
                }
            }
        }
        VkInstance->mExtensionsCount = filled_exts;
    }
    return;
}

void VkUtil_SelectPhysicalDeviceLayers(struct CGPUAdapter_Vulkan* VkAdapter,
const char* const* device_layers, uint32_t device_layers_count)
{
    uint32_t count;
    vkEnumerateDeviceLayerProperties(VkAdapter->pPhysicalDevice, &count, NULL);
    if (count != 0)
    {
        VkAdapter->pLayerNames = cgpu_calloc(device_layers_count, sizeof(const char*));
        VkAdapter->pLayerProperties = cgpu_calloc(device_layers_count, sizeof(VkLayerProperties));

        DECLARE_ZERO_VLA(VkLayerProperties, layer_props, count)
        vkEnumerateDeviceLayerProperties(VkAdapter->pPhysicalDevice, &count, layer_props);
        uint32_t filled_exts = 0;
        for (uint32_t i = 0; i < count; i++)
        {
            for (uint32_t j = 0; j < device_layers_count; j++)
            {
                if (strcmp(layer_props[i].layerName, device_layers[j]) == 0)
                {
                    VkAdapter->pLayerProperties[filled_exts] = layer_props[i];
                    VkAdapter->pLayerNames[filled_exts] = VkAdapter->pLayerProperties[filled_exts].layerName;
                    filled_exts++;
                    break;
                }
            }
        }
        VkAdapter->mLayersCount = filled_exts;
    }
    return;
}

void VkUtil_SelectPhysicalDeviceExtensions(struct CGPUAdapter_Vulkan* VkAdapter,
const char* const* device_extensions, uint32_t device_extension_count)
{
    const char* layer_name = NULL; // Query Vulkan implementation or by implicitly enabled layers
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(VkAdapter->pPhysicalDevice, layer_name, &count, NULL);
    if (count > 0)
    {
        VkAdapter->pExtensionProperties = cgpu_calloc(device_extension_count, sizeof(VkExtensionProperties));
        VkAdapter->pExtensionNames = cgpu_calloc(device_extension_count, sizeof(const char*));

        DECLARE_ZERO_VLA(VkExtensionProperties, ext_props, count)
        vkEnumerateDeviceExtensionProperties(VkAdapter->pPhysicalDevice, layer_name, &count, ext_props);
        uint32_t filled_exts = 0;
        for (uint32_t i = 0; i < count; i++)
        {
            for (uint32_t j = 0; j < device_extension_count; j++)
            {
                if (strcmp(ext_props[i].extensionName, device_extensions[j]) == 0)
                {
                    VkAdapter->pExtensionProperties[filled_exts] = ext_props[i];
                    VkAdapter->pExtensionNames[filled_exts] = VkAdapter->pExtensionProperties[filled_exts].extensionName;
                    filled_exts++;
                    continue;
                }
            }
        }
        VkAdapter->mExtensionsCount = filled_exts;
    }
    return;
}

// Debug Callback
FORCEINLINE static void VkUtil_DebugUtilsSetObjectName(VkDevice pDevice, uint64_t handle,
VkObjectType type, const char* pName)
{
    VkDebugUtilsObjectNameInfoEXT nameInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = type,
        .objectHandle = handle,
        .pObjectName = pName
    };
    vkSetDebugUtilsObjectNameEXT(pDevice, &nameInfo);
}

FORCEINLINE static void VkUtil_DebugReportSetObjectName(VkDevice pDevice, uint64_t handle,
VkDebugReportObjectTypeEXT type, const char* pName)
{
    VkDebugMarkerObjectNameInfoEXT nameInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT,
        .objectType = type,
        .object = (uint64_t)handle,
        .pObjectName = pName
    };
    vkDebugMarkerSetObjectNameEXT(pDevice, &nameInfo);
}

void VkUtil_OptionalSetObjectName(struct CGPUDevice_Vulkan* device, uint64_t handle, VkObjectType type, const char* name)
{
    CGPUInstance_Vulkan* I = (CGPUInstance_Vulkan*)device->super.adapter->instance;
    if (I->super.enable_set_name && name)
    {
        if (I->debug_report)
        {
            VkDebugReportObjectTypeEXT exttype = VkUtil_ObjectTypeToDebugReportType(type);
            VkUtil_DebugReportSetObjectName(device->pVkDevice, handle, exttype, name);
        }
        if (I->debug_utils)
        {
            VkUtil_DebugUtilsSetObjectName(device->pVkDevice, handle, type, name);
        }
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL
VkUtil_DebugUtilsCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
VkDebugUtilsMessageTypeFlagsEXT messageType,
const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
void* pUserData)
{
    switch (messageSeverity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            cgpu_trace("Vulkan validation layer: %s\n", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            cgpu_info("Vulkan validation layer: %s\n", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            cgpu_warn("Vulkan validation layer: %s\n", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            cgpu_error("Vulkan validation layer: %s\n", pCallbackData->pMessage);
            break;
        default:
            return VK_TRUE;
    }
    return VK_FALSE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
VkUtil_DebugReportCallback(
VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
uint64_t object, size_t location, int32_t messageCode,
const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    switch (flags)
    {
        case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
            cgpu_info("Vulkan validation layer: %s\n", pMessage);
        case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
            cgpu_warn("Vulkan validation layer: %s\n", pMessage);
            break;
        case VK_DEBUG_REPORT_WARNING_BIT_EXT:
            cgpu_warn("Vulkan validation layer: %s\n", pMessage);
            break;
        case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
            cgpu_debug("Vulkan validation layer: %s\n", pMessage);
            break;
        case VK_DEBUG_REPORT_ERROR_BIT_EXT:
            cgpu_error("Vulkan validation layer: %s\n", pMessage);
            break;
        default:
            return VK_TRUE;
    }
    return VK_FALSE;
}
