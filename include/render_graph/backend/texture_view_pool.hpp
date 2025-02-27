#pragma once
#include <EASTL/unordered_map.h>
#include "utils/hash.h"
#include "cgpu/api.h"

namespace skr
{
namespace render_graph
{
class TextureViewPool
{
public:
    struct Key {
        CGPUDeviceId device;
        CGPUTextureId texture;
        ECGPUFormat format;
        CGPUTexutreViewUsages usages;
        CGPUTextureViewAspects aspects;
        ECGPUTextureDimension dims;
        uint32_t base_array_layer;
        uint32_t array_layer_count;
        uint32_t base_mip_level;
        uint32_t mip_level_count;
        operator size_t() const;
        friend class TextureViewPool;

    protected:
        Key(CGPUDeviceId device, const CGPUTextureViewDescriptor& desc);
    };
    friend class RenderGraphBackend;
    void initialize(CGPUDeviceId device);
    void finalize();
    uint32_t erase(CGPUTextureId texture);
    CGPUTextureViewId allocate(const CGPUTextureViewDescriptor& desc, uint64_t frame_index);

protected:
    CGPUDeviceId device;
    eastl::unordered_map<Key, eastl::pair<CGPUTextureViewId, uint64_t>> views;
};

inline TextureViewPool::Key::Key(CGPUDeviceId device, const CGPUTextureViewDescriptor& desc)
    : device(device)
    , texture(desc.texture)
    , format(desc.format)
    , usages(desc.usages)
    , aspects(desc.aspects)
    , dims(desc.dims)
    , base_array_layer(desc.base_array_layer)
    , array_layer_count(desc.array_layer_count)
    , base_mip_level(desc.base_mip_level)
    , mip_level_count(desc.mip_level_count)
{
}

inline uint32_t TextureViewPool::erase(CGPUTextureId texture)
{
    auto prev_size = (uint32_t)views.size();
    for (auto it = views.begin(); it != views.end();)
    {
        if (it->first.texture == texture)
        {
            cgpu_free_texture_view(it->second.first);
            it = views.erase(it);
        }
        else
            ++it;
    }
    return prev_size - (uint32_t)views.size();
}

inline TextureViewPool::Key::operator size_t() const
{
    return skr_hash(this, sizeof(*this), (size_t)device);
}

inline void TextureViewPool::initialize(CGPUDeviceId device_)
{
    device = device_;
}

inline void TextureViewPool::finalize()
{
    for (auto&& view : views)
    {
        cgpu_free_texture_view(view.second.first);
    }
    views.clear();
}

inline CGPUTextureViewId TextureViewPool::allocate(const CGPUTextureViewDescriptor& desc, uint64_t frame_index)
{
    const TextureViewPool::Key key(device, desc);
    auto&& found = views.find(key);
    if (found != views.end() && found->first.texture == key.texture)
    {
        found->second.second = frame_index;
        return found->second.first;
    }
    else
    {
        CGPUTextureViewId new_view = cgpu_create_texture_view(device, &desc);
        views[key] = { new_view, frame_index };
        return new_view;
    }
}
} // namespace render_graph
} // namespace skr