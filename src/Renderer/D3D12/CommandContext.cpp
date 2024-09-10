
#include "pch.h"
#include "CommandContext.h"

using namespace Fyuu::graphics::d3d12;

static std::array<std::list<std::shared_ptr<CommandContext>>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> s_contexts;
static std::array<std::queue<std::shared_ptr<CommandContext>>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> s_available_contexts;
static std::mutex s_context_allocation_mutex;

Fyuu::graphics::d3d12::CommandContext::CommandContext(D3D12_COMMAND_LIST_TYPE type)
	:m_type(type) {



}

void Fyuu::graphics::d3d12::CommandContext::Reset() {

	m_cmd_object.allocator = Queue(m_type)->RequestAllocator();
	m_cmd_object.list->Reset(m_cmd_object.allocator.Get(), nullptr);

	m_cur_compute_root_signature = nullptr;
	m_cur_graphics_root_signature = nullptr;
	m_cur_pipeline_state = nullptr;
	m_num_barriers_to_flush = 0;

	BindDescriptorHeaps();

}

void Fyuu::graphics::d3d12::CommandContext::BindDescriptorHeaps() {

	std::uint32_t non_null_heaps = 0;
	std::array<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> heaps_to_bind = {};
	for (auto const& heap : m_current_descriptor_heaps) {
		heaps_to_bind[non_null_heaps++] = heap.Get();
	}

	if (non_null_heaps) {
		m_cmd_object.list->SetDescriptorHeaps(non_null_heaps, heaps_to_bind.data());
	}

}

std::shared_ptr<CommandContext> Fyuu::graphics::d3d12::AllocateCommandContext(D3D12_COMMAND_LIST_TYPE type) {
	
	std::lock_guard<std::mutex> lock(s_context_allocation_mutex);

	std::shared_ptr<CommandContext> ret = nullptr;

	if (s_available_contexts.empty()) {
		ret = std::make_shared<CommandContext>(type);
	}
	else {
		ret = std::move(s_available_contexts[type].front());
		s_available_contexts[type].pop();
		ret->Reset();
	}

	return ret;

}

void Fyuu::graphics::d3d12::FreeCommandContext(std::shared_ptr<CommandContext>& context) {

	std::lock_guard<std::mutex> lock(s_context_allocation_mutex);

	s_available_contexts[context->GetCommandListType()].push(std::move(context));

}

void Fyuu::graphics::d3d12::InitializeTexture(GPUResource& dest, std::uint32_t num_sub_resources, D3D12_SUBRESOURCE_DATA* sub_data) {

	std::size_t upload_buffer_size = 0;
	auto desc = dest.GetResource()->GetDesc();
	D3D12Device()->GetCopyableFootprints(&desc, 0, num_sub_resources, 0, nullptr, nullptr, nullptr, &upload_buffer_size);

}
