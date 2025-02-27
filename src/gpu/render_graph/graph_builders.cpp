#include "render_graph/frontend/render_graph.hpp"
#include "../cgpu/common/common_utils.h"

namespace skr
{
namespace render_graph
{
// graph builder
RenderGraph::RenderPassBuilder::RenderPassBuilder(RenderGraph& graph, RenderPassNode& pass) SKR_NOEXCEPT
    : graph(graph),
      node(pass)
{
}
RenderGraph::RenderGraphBuilder& RenderGraph::RenderGraphBuilder::with_device(CGPUDeviceId device_) SKR_NOEXCEPT
{
    device = device_;
    return *this;
}
RenderGraph::RenderGraphBuilder& RenderGraph::RenderGraphBuilder::enable_memory_aliasing() SKR_NOEXCEPT
{
    memory_aliasing = true;
    return *this;
}
RenderGraph::RenderGraphBuilder& RenderGraph::RenderGraphBuilder::with_gfx_queue(CGPUQueueId queue) SKR_NOEXCEPT
{
    gfx_queue = queue;
    return *this;
}
RenderGraph::RenderGraphBuilder& RenderGraph::RenderGraphBuilder::backend_api(ECGPUBackend backend) SKR_NOEXCEPT
{
    api = backend;
    return *this;
}
RenderGraph::RenderGraphBuilder& RenderGraph::RenderGraphBuilder::frontend_only() SKR_NOEXCEPT
{
    no_backend = true;
    return *this;
}
PassHandle RenderGraph::add_render_pass(const RenderPassSetupFunction& setup, const RenderPassExecuteFunction& executor) SKR_NOEXCEPT
{
    auto newPass = new RenderPassNode((uint32_t)passes.size());
    passes.emplace_back(newPass);
    graph->insert(newPass);
    // build up
    RenderPassBuilder builder(*this, *newPass);
    setup(*this, builder);
    newPass->executor = executor;
    return newPass->get_handle();
}

// render pass builder
RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::set_name(const char* name) SKR_NOEXCEPT
{
    if (name)
    {
        graph.blackboard.named_passes[name] = &node;
        node.set_name(name);
    }
    return *this;
}
RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::read(uint32_t set, uint32_t binding, TextureSRVHandle handle) SKR_NOEXCEPT
{
    auto&& edge = node.in_texture_edges.emplace_back(
    new TextureReadEdge(set, binding, handle));
    graph.graph->link(graph.graph->access_node(handle._this), &node, edge);
    return *this;
}
RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::read(const char8_t* name, TextureSRVHandle handle) SKR_NOEXCEPT
{
    auto&& edge = node.in_texture_edges.emplace_back(
    new TextureReadEdge(name, handle));
    graph.graph->link(graph.graph->access_node(handle._this), &node, edge);
    return *this;
}
RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::write(
uint32_t mrt_index, TextureRTVHandle handle, ECGPULoadAction load_action,
ECGPUStoreAction store_action) SKR_NOEXCEPT
{
    auto&& edge = node.out_texture_edges.emplace_back(
    new TextureRenderEdge(mrt_index, handle._this));
    graph.graph->link(&node, graph.graph->access_node(handle._this), edge);
    node.load_actions[mrt_index] = load_action;
    node.store_actions[mrt_index] = store_action;
    return *this;
}
RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::set_depth_stencil(TextureDSVHandle handle,
ECGPULoadAction dload_action, ECGPUStoreAction dstore_action,
ECGPULoadAction sload_action, ECGPUStoreAction sstore_action) SKR_NOEXCEPT
{
    auto&& edge = node.out_texture_edges.emplace_back(
    new TextureRenderEdge(
    CGPU_MAX_MRT_COUNT, handle._this,
    CGPU_RESOURCE_STATE_DEPTH_WRITE));
    graph.graph->link(&node, graph.graph->access_node(handle._this), edge);
    node.depth_load_action = dload_action;
    node.depth_store_action = dstore_action;
    node.stencil_load_action = sload_action;
    node.stencil_store_action = sstore_action;
    return *this;
}
RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::read(uint32_t set, uint32_t binding, BufferHandle handle) SKR_NOEXCEPT
{
    return *this;
}
RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::read(const char8_t* name, BufferHandle handle) SKR_NOEXCEPT
{
    return *this;
}
RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::write(uint32_t set, uint32_t binding, BufferHandle handle) SKR_NOEXCEPT
{
    return *this;
}
RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::write(const char8_t* name, BufferHandle handle) SKR_NOEXCEPT
{
    return *this;
}
RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::use_buffer(PipelineBufferHandle buffer, ECGPUResourceState requested_state) SKR_NOEXCEPT
{
    auto&& edge = node.ppl_buffer_edges.emplace_back(
    new PipelineBufferEdge(buffer, requested_state));
    graph.graph->link(graph.graph->access_node(buffer._this), &node, edge);
    return *this;
}
RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::set_pipeline(CGPURenderPipelineId pipeline) SKR_NOEXCEPT
{
    node.pipeline = pipeline;
    node.root_signature = pipeline->root_signature;
    return *this;
}
RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::set_root_signature(CGPURootSignatureId signature) SKR_NOEXCEPT
{
    node.root_signature = signature;
    return *this;
}

// compute pass builder
RenderGraph::ComputePassBuilder::ComputePassBuilder(RenderGraph& graph, ComputePassNode& pass) SKR_NOEXCEPT
    : graph(graph),
      node(pass)
{
}
RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::set_name(const char* name) SKR_NOEXCEPT
{
    if (name)
    {
        graph.blackboard.named_passes[name] = &node;
        node.set_name(name);
    }
    return *this;
}
RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::read(uint32_t set, uint32_t binding, TextureSRVHandle handle) SKR_NOEXCEPT
{
    auto&& edge = node.in_texture_edges.emplace_back(
    new TextureReadEdge(set, binding, handle));
    graph.graph->link(graph.graph->access_node(handle._this), &node, edge);
    return *this;
}
RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::read(const char8_t* name, TextureSRVHandle handle) SKR_NOEXCEPT
{
    auto&& edge = node.in_texture_edges.emplace_back(
    new TextureReadEdge(name, handle));
    graph.graph->link(graph.graph->access_node(handle._this), &node, edge);
    return *this;
}
RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::readwrite(uint32_t set, uint32_t binding, TextureUAVHandle handle) SKR_NOEXCEPT
{
    auto&& edge = node.inout_texture_edges.emplace_back(
    new TextureReadWriteEdge(set, binding, handle));
    graph.graph->link(&node, graph.graph->access_node(handle._this), edge);
    return *this;
}
RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::readwrite(const char8_t* name, TextureUAVHandle handle) SKR_NOEXCEPT
{
    auto&& edge = node.inout_texture_edges.emplace_back(
    new TextureReadWriteEdge(name, handle));
    graph.graph->link(&node, graph.graph->access_node(handle._this), edge);
    return *this;
}
RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::read(uint32_t set, uint32_t binding, BufferHandle handle) SKR_NOEXCEPT
{
    return *this;
}
RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::read(const char8_t* name, BufferHandle handle) SKR_NOEXCEPT
{
    return *this;
}
RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::readwrite(uint32_t set, uint32_t binding, BufferHandle handle) SKR_NOEXCEPT
{
    return *this;
}
RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::readwrite(const char8_t* name, BufferHandle handle) SKR_NOEXCEPT
{
    return *this;
}
RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::set_pipeline(CGPUComputePipelineId pipeline) SKR_NOEXCEPT
{
    node.pipeline = pipeline;
    node.root_signature = pipeline->root_signature;
    return *this;
}
RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::set_root_signature(CGPURootSignatureId signature) SKR_NOEXCEPT
{
    node.root_signature = signature;
    return *this;
}
PassHandle RenderGraph::add_compute_pass(const ComputePassSetupFunction& setup, const ComputePassExecuteFunction& executor) SKR_NOEXCEPT
{
    auto newPass = new ComputePassNode((uint32_t)passes.size());
    passes.emplace_back(newPass);
    graph->insert(newPass);
    // build up
    ComputePassBuilder builder(*this, *newPass);
    setup(*this, builder);
    newPass->executor = executor;
    return newPass->get_handle();
}

// copy pass
RenderGraph::CopyPassBuilder::CopyPassBuilder(RenderGraph& graph, CopyPassNode& pass) SKR_NOEXCEPT
    : graph(graph),
      node(pass)
{
}
RenderGraph::CopyPassBuilder& RenderGraph::CopyPassBuilder::set_name(const char* name) SKR_NOEXCEPT
{
    if (name)
    {
        graph.blackboard.named_passes[name] = &node;
        node.set_name(name);
    }
    return *this;
}
RenderGraph::CopyPassBuilder& RenderGraph::CopyPassBuilder::buffer_to_buffer(BufferRangeHandle src, BufferRangeHandle dst) SKR_NOEXCEPT
{
    auto&& in_edge = node.in_buffer_edges.emplace_back(
    new BufferReadEdge(src, CGPU_RESOURCE_STATE_COPY_SOURCE));
    auto&& out_edge = node.out_buffer_edges.emplace_back(
    new BufferReadWriteEdge(dst, CGPU_RESOURCE_STATE_COPY_DEST));
    graph.graph->link(graph.graph->access_node(src._this), &node, in_edge);
    graph.graph->link(&node, graph.graph->access_node(dst._this), out_edge);
    node.b2bs.emplace_back(src, dst);
    return *this;
}
RenderGraph::CopyPassBuilder& RenderGraph::CopyPassBuilder::texture_to_texture(TextureSubresourceHandle src, TextureSubresourceHandle dst) SKR_NOEXCEPT
{
    auto&& in_edge = node.in_texture_edges.emplace_back(
    new TextureReadEdge(0, 0, src._this,
    CGPU_RESOURCE_STATE_COPY_SOURCE));
    auto&& out_edge = node.out_texture_edges.emplace_back(
    new TextureRenderEdge(0, dst._this,
    CGPU_RESOURCE_STATE_COPY_DEST));
    graph.graph->link(graph.graph->access_node(src._this), &node, in_edge);
    graph.graph->link(&node, graph.graph->access_node(dst._this), out_edge);
    node.t2ts.emplace_back(src, dst);
    return *this;
}
PassHandle RenderGraph::add_copy_pass(const CopyPassSetupFunction& setup) SKR_NOEXCEPT
{
    auto newPass = new CopyPassNode((uint32_t)passes.size());
    passes.emplace_back(newPass);
    graph->insert(newPass);
    // build up
    CopyPassBuilder builder(*this, *newPass);
    setup(*this, builder);
    return newPass->get_handle();
}

// present pass builder
RenderGraph::PresentPassBuilder::PresentPassBuilder(RenderGraph& graph, PresentPassNode& present) SKR_NOEXCEPT
    : graph(graph),
      node(present)
{
}
RenderGraph::PresentPassBuilder& RenderGraph::PresentPassBuilder::set_name(const char* name) SKR_NOEXCEPT
{
    if (name)
    {
        graph.blackboard.named_passes[name] = &node;
        node.set_name(name);
    }
    return *this;
}
RenderGraph::PresentPassBuilder& RenderGraph::PresentPassBuilder::swapchain(CGPUSwapChainId chain, uint32_t index) SKR_NOEXCEPT
{
    node.descriptor.swapchain = chain;
    node.descriptor.index = index;
    return *this;
}
RenderGraph::PresentPassBuilder& RenderGraph::PresentPassBuilder::texture(TextureHandle handle, bool is_backbuffer) SKR_NOEXCEPT
{
    assert(is_backbuffer && "blit to screen mode not supported!");
    auto&& edge = node.in_texture_edges.emplace_back(
    new TextureReadEdge(0, 0, handle, CGPU_RESOURCE_STATE_PRESENT));
    graph.graph->link(graph.graph->access_node(handle), &node, edge);
    return *this;
}
PassHandle RenderGraph::add_present_pass(const PresentPassSetupFunction& setup) SKR_NOEXCEPT
{
    auto newPass = new PresentPassNode((uint32_t)passes.size());
    passes.emplace_back(newPass);
    graph->insert(newPass);
    // build up
    PresentPassBuilder builder(*this, *newPass);
    setup(*this, builder);
    return newPass->get_handle();
}

// buffer builder
RenderGraph::BufferBuilder::BufferBuilder(RenderGraph& graph, BufferNode& node) SKR_NOEXCEPT
    : graph(graph),
      node(node)
{
    node.descriptor.descriptors = CGPU_RESOURCE_TYPE_NONE;
    node.descriptor.flags = CGPU_BCF_NONE;
    node.descriptor.memory_usage = CGPU_MEM_USAGE_GPU_ONLY;
}
RenderGraph::BufferBuilder& RenderGraph::BufferBuilder::set_name(const char* name) SKR_NOEXCEPT
{
    node.descriptor.name = name;
    // blackboard
    graph.blackboard.named_buffers[name] = &node;
    node.set_name(name);
    return *this;
}
RenderGraph::BufferBuilder& RenderGraph::BufferBuilder::import(CGPUBufferId buffer, ECGPUResourceState init_state) SKR_NOEXCEPT
{
    node.imported = buffer;
    node.frame_buffer = buffer;
    node.init_state = init_state;
    node.descriptor.descriptors = buffer->descriptors;
    node.descriptor.size = buffer->size;
    node.descriptor.memory_usage = (ECGPUMemoryUsage)buffer->memory_usage;
    return *this;
}
RenderGraph::BufferBuilder& RenderGraph::BufferBuilder::owns_memory() SKR_NOEXCEPT
{
    node.descriptor.flags |= CGPU_BCF_OWN_MEMORY_BIT;
    return *this;
}
RenderGraph::BufferBuilder& RenderGraph::BufferBuilder::structured(uint64_t first_element, uint64_t element_count, uint64_t element_stride) SKR_NOEXCEPT
{
    node.descriptor.first_element = first_element;
    node.descriptor.elemet_count = element_count;
    node.descriptor.element_stride = element_stride;
    return *this;
}
RenderGraph::BufferBuilder& RenderGraph::BufferBuilder::size(uint64_t size) SKR_NOEXCEPT
{
    node.descriptor.size = size;
    return *this;
}
RenderGraph::BufferBuilder& RenderGraph::BufferBuilder::with_flags(CGPUBufferCreationFlags flags) SKR_NOEXCEPT
{
    node.descriptor.flags |= flags;
    return *this;
}
RenderGraph::BufferBuilder& RenderGraph::BufferBuilder::memory_usage(ECGPUMemoryUsage mem_usage) SKR_NOEXCEPT
{
    node.descriptor.memory_usage = mem_usage;
    return *this;
}
RenderGraph::BufferBuilder& RenderGraph::BufferBuilder::allow_shader_readwrite() SKR_NOEXCEPT
{
    node.descriptor.descriptors |= CGPU_RESOURCE_TYPE_RW_BUFFER;
    return *this;
}
RenderGraph::BufferBuilder& RenderGraph::BufferBuilder::allow_shader_read() SKR_NOEXCEPT
{
    node.descriptor.descriptors |= CGPU_RESOURCE_TYPE_BUFFER;
    node.descriptor.descriptors |= CGPU_RESOURCE_TYPE_UNIFORM_BUFFER;
    return *this;
}
RenderGraph::BufferBuilder& RenderGraph::BufferBuilder::as_upload_buffer() SKR_NOEXCEPT
{
    node.descriptor.flags |= CGPU_BCF_PERSISTENT_MAP_BIT;
    node.descriptor.start_state = CGPU_RESOURCE_STATE_COPY_SOURCE;
    node.descriptor.memory_usage = CGPU_MEM_USAGE_CPU_ONLY;
    return *this;
}
RenderGraph::BufferBuilder& RenderGraph::BufferBuilder::as_vertex_buffer() SKR_NOEXCEPT
{
    node.descriptor.descriptors |= CGPU_RESOURCE_TYPE_VERTEX_BUFFER;
    node.descriptor.start_state = CGPU_RESOURCE_STATE_COPY_DEST;
    return *this;
}
RenderGraph::BufferBuilder& RenderGraph::BufferBuilder::as_index_buffer() SKR_NOEXCEPT
{
    node.descriptor.descriptors |= CGPU_RESOURCE_TYPE_INDEX_BUFFER;
    node.descriptor.start_state = CGPU_RESOURCE_STATE_COPY_DEST;
    return *this;
}
RenderGraph::BufferBuilder& RenderGraph::BufferBuilder::prefer_on_device() SKR_NOEXCEPT
{
    node.descriptor.prefer_on_device = true;
    return *this;
}
RenderGraph::BufferBuilder& RenderGraph::BufferBuilder::prefer_on_host() SKR_NOEXCEPT
{
    node.descriptor.prefer_on_device = true;
    return *this;
}
BufferHandle RenderGraph::create_buffer(const BufferSetupFunction& setup) SKR_NOEXCEPT
{
    auto newTex = new BufferNode();
    resources.emplace_back(newTex);
    graph->insert(newTex);
    BufferBuilder builder(*this, *newTex);
    setup(*this, builder);
    return newTex->get_handle();
}
BufferHandle RenderGraph::get_buffer(const char* name) SKR_NOEXCEPT
{
    if (blackboard.named_buffers.find(name) != blackboard.named_buffers.end())
        return blackboard.named_buffers[name]->get_handle();
    return UINT64_MAX;
}

// texture builder
RenderGraph::TextureBuilder::TextureBuilder(RenderGraph& graph, TextureNode& node) SKR_NOEXCEPT
    : graph(graph),
      node(node)
{
    node.descriptor.descriptors = CGPU_RESOURCE_TYPE_TEXTURE;
    node.descriptor.is_dedicated = false;
}
RenderGraph::TextureBuilder& RenderGraph::TextureBuilder::set_name(const char* name) SKR_NOEXCEPT
{
    node.descriptor.name = name;
    // blackboard
    graph.blackboard.named_textures[name] = &node;
    node.set_name(name);
    return *this;
}
RenderGraph::TextureBuilder& RenderGraph::TextureBuilder::import(CGPUTextureId texture, ECGPUResourceState init_state) SKR_NOEXCEPT
{
    node.imported = texture;
    node.frame_texture = texture;
    node.init_state = init_state;
    node.descriptor.width = texture->width;
    node.descriptor.height = texture->height;
    node.descriptor.depth = texture->depth;
    node.descriptor.format = (ECGPUFormat)texture->format;
    node.descriptor.array_size = texture->array_size_minus_one + 1;
    node.descriptor.sample_count = texture->sample_count;
    return *this;
}
RenderGraph::TextureBuilder& RenderGraph::TextureBuilder::extent(
uint32_t width, uint32_t height, uint32_t depth) SKR_NOEXCEPT
{
    node.descriptor.width = width;
    node.descriptor.height = height;
    node.descriptor.depth = depth;
    return *this;
}
RenderGraph::TextureBuilder& RenderGraph::TextureBuilder::format(
ECGPUFormat format) SKR_NOEXCEPT
{
    node.descriptor.format = format;
    return *this;
}
RenderGraph::TextureBuilder& RenderGraph::TextureBuilder::array(uint32_t size) SKR_NOEXCEPT
{
    node.descriptor.array_size = size;
    return *this;
}
RenderGraph::TextureBuilder& RenderGraph::TextureBuilder::sample_count(
ECGPUSampleCount count) SKR_NOEXCEPT
{
    node.descriptor.sample_count = count;
    return *this;
}
RenderGraph::TextureBuilder& RenderGraph::TextureBuilder::allow_readwrite() SKR_NOEXCEPT
{
    node.descriptor.descriptors |= CGPU_RESOURCE_TYPE_RW_TEXTURE;
    node.descriptor.start_state = CGPU_RESOURCE_STATE_UNDEFINED;
    return *this;
}
RenderGraph::TextureBuilder& RenderGraph::TextureBuilder::allow_render_target() SKR_NOEXCEPT
{
    node.descriptor.descriptors |= CGPU_RESOURCE_TYPE_RENDER_TARGET;
    node.descriptor.start_state = CGPU_RESOURCE_STATE_UNDEFINED;
    return *this;
}
RenderGraph::TextureBuilder& RenderGraph::TextureBuilder::allow_depth_stencil() SKR_NOEXCEPT
{
    node.descriptor.descriptors |= CGPU_RESOURCE_TYPE_DEPTH_STENCIL;
    node.descriptor.start_state = CGPU_RESOURCE_STATE_UNDEFINED;
    return *this;
}

RenderGraph::TextureBuilder& RenderGraph::TextureBuilder::owns_memory() SKR_NOEXCEPT
{
    node.descriptor.flags |= CGPU_TCF_OWN_MEMORY_BIT;
    return *this;
}
RenderGraph::TextureBuilder& RenderGraph::TextureBuilder::allow_lone() SKR_NOEXCEPT
{
    node.canbe_lone = true;
    return *this;
}
TextureHandle RenderGraph::create_texture(const TextureSetupFunction& setup) SKR_NOEXCEPT
{
    auto newTex = new TextureNode();
    resources.emplace_back(newTex);
    graph->insert(newTex);
    TextureBuilder builder(*this, *newTex);
    setup(*this, builder);
    return newTex->get_handle();
}
TextureHandle RenderGraph::get_texture(const char* name) SKR_NOEXCEPT
{
    if (blackboard.named_textures.find(name) != blackboard.named_textures.end())
        return blackboard.named_textures[name]->get_handle();
    return UINT64_MAX;
}
} // namespace render_graph
} // namespace skr