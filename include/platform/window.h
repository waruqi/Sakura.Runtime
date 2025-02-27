#pragma once
#include "configure.h"

typedef struct SWindow SWindow;
typedef SWindow* SWindowHandle;

typedef struct SWindowDescroptor {
    uint32_t width;
    uint32_t height;
    uint32_t posx;
    uint32_t posy;
    bool centered;
    bool resizable;
} SWindowDescroptor;

#ifdef __cplusplus
extern "C" {
#endif
RUNTIME_API SWindowHandle skr_create_window(const char8_t* name, const SWindowDescroptor* desc);
RUNTIME_API void skr_window_set_title(SWindowHandle window, const char8_t* name);
RUNTIME_API void skr_window_get_extent(SWindowHandle window, int32_t* width, int32_t* height);
RUNTIME_API bool skr_window_is_focused(SWindowHandle window);
RUNTIME_API SWindowHandle skr_get_mouse_focused_window();
RUNTIME_API void* skr_window_get_native_handle(SWindowHandle window);
RUNTIME_API void* skr_window_get_native_view(SWindowHandle window);
RUNTIME_API void skr_free_window(SWindowHandle window);
#ifdef __cplusplus
}
#endif