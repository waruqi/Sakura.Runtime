#pragma once
#include "cgpu/api.h"
#include "platform/memory.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CGPURuntimeTable* cgpu_create_runtime_table();
void cgpu_free_runtime_table(struct CGPURuntimeTable* table);
void cgpu_runtime_table_add_queue(CGPUQueueId queue, ECGPUQueueType type, uint32_t index);
CGPUQueueId cgpu_runtime_table_try_get_queue(CGPUDeviceId device, ECGPUQueueType type, uint32_t index);

void CGPUUtil_InitRSParamTables(CGPURootSignature* RS, const struct CGPURootSignatureDescriptor* desc);
void CGPUUtil_FreeRSParamTables(CGPURootSignature* RS);

// check for slot-overlapping and try get a signature from pool
CGPURootSignaturePoolId CGPUUtil_CreateRootSignaturePool(const CGPURootSignaturePoolDescriptor* desc);
CGPURootSignatureId CGPUUtil_TryAllocateSignature(CGPURootSignaturePoolId pool, CGPURootSignature* RSTables, const struct CGPURootSignatureDescriptor* desc);
bool CGPUUtil_AddSignature(CGPURootSignaturePoolId pool, CGPURootSignature* sig, const CGPURootSignatureDescriptor* desc);
// TODO: signature pool statics
//void CGPUUtil_AllSignatures(CGPURootSignaturePoolId pool, CGPURootSignatureId* signatures, uint32_t* count);
bool CGPUUtil_PoolFreeSignature(CGPURootSignaturePoolId pool, CGPURootSignatureId sig);
void CGPUUtil_FreeRootSignaturePool(CGPURootSignaturePoolId pool);

#ifdef __cplusplus
} // end extern "C"
#endif

#include "utils/hash.h"
#define cgpu_hash(buffer, size, seed) skr_hash((buffer), (size), (seed))
#include "utils/log.h"
#define cgpu_trace(...) log_log(SKR_LOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define cgpu_debug(...) log_log(SKR_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define cgpu_info(...) log_log(SKR_LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define cgpu_warn(...) log_log(SKR_LOG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define cgpu_error(...) log_log(SKR_LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define cgpu_fatal(...) log_log(SKR_LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#ifdef _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
FORCEINLINE static void* _aligned_calloc(size_t nelem, size_t elsize, size_t alignment)
{
    void* memory = _aligned_malloc(nelem * elsize, alignment);
    if (memory != NULL) memset(memory, 0, nelem * elsize);
    return memory;
}
    #define cgpu_malloc malloc
    #define cgpu_malloc_aligned _aligned_malloc
    #define cgpu_calloc calloc
    #define cgpu_calloc_aligned _aligned_calloc
    #define cgpu_memalign _aligned_malloc
    #define cgpu_free free
    #define cgpu_free_aligned _aligned_free
#else
    #define cgpu_malloc sakura_malloc
    #define cgpu_malloc_aligned sakura_malloc_aligned
    #define cgpu_calloc sakura_calloc
    #define cgpu_calloc_aligned sakura_calloc_aligned
    #define cgpu_memalign sakura_malloc_aligned
    #define cgpu_free sakura_free
    #define cgpu_free_aligned sakura_free
#endif

#ifdef __cplusplus
    #include <type_traits>
template <typename T, typename... Args>
T* cgpu_new_placed(void* memory, Args&&... args)
{
    return new (memory) T(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
T* cgpu_new(Args&&... args)
{
    return SkrNew<T>(std::forward<Args>(args)...);
}
template <typename T>
void cgpu_delete_placed(T* object)
{
    object->~T();
}
template <typename T>
void cgpu_delete(T* object)
{
    SkrDelete(object);
}
#endif
