#pragma once
#include "configure.h"
RUNTIME_API void skr_debug_output(const char* msg);

#if SKR_SHIPPING
// Platform specific, need to be defined in Platform/Defines_XXX.h

    #define SKR_ASSERT(cond)
    #define SKR_BREAK()
    #define SKR_HALT()

    #define SKR_DISABLE_OPTIMIZATION
    #define SKR_ENABLE_OPTIMIZATION

// Platform agnostic asserts
//-------------------------------------------------------------------------

    #define SKR_TRACE_MSG(msg)
    #define SKR_TRACE_ASSERT(msg)
    #define SKR_UNIMPLEMENTED_FUNCTION()
    #define SKR_UNREACHABLE_CODE()
#else
    #define SKR_TRACE_ASSERT(msg) \
        {                         \
            SKR_TRACE_MSG(msg);   \
            SKR_HALT();           \
        }
    #define SKR_UNIMPLEMENTED_FUNCTION() SKR_TRACE_ASSERT("Function not implemented!\n")
    #define SKR_UNREACHABLE_CODE() SKR_TRACE_ASSERT("Unreachable code encountered!\n")
#endif