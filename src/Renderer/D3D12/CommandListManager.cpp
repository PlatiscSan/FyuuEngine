
#include "pch.h"
#include "CommandListManager.h"
#include "Platform/Windows/WindowsUtility.h"

#ifdef max
	#undef max
#endif // max

using namespace Fyuu::utility::windows;
using namespace Fyuu::graphics::d3d12;

static std::shared_ptr<CommandQueue> s_graphics_queue;
static std::shared_ptr<CommandQueue> s_compute_queue;
static std::shared_ptr<CommandQueue> s_copy_queue;

Fyuu::graphics::d3d12::CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type, Microsoft::WRL::ComPtr<ID3D12Device10> const& device) {

	m_type = type;
	m_next_fence_value = static_cast<std::uint64_t>(m_type) << 56 | 1;
	m_last_completed_fence_value = static_cast<std::uint64_t>(m_type) << 56;

	D3D12_COMMAND_QUEUE_DESC queue_desc = {};
	queue_desc.Type = m_type;
	queue_desc.NodeMask = 1;
	ThrowIfFailed(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_command_queue)));

	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fence->Signal(static_cast<std::uint64_t>(m_type) << 56);

	m_fence_event = CreateEvent(nullptr, false, false, nullptr);
	if (!m_fence_event) {
		throw std::runtime_error(GetLastErrorFromWinAPI());
	}

	m_allocator_pool = std::make_unique<CommandAllocatorPool>(m_type, device);

}

Fyuu::graphics::d3d12::CommandQueue::~CommandQueue() noexcept {

	CloseHandle(m_fence_event);

}

std::uint64_t Fyuu::graphics::d3d12::CommandQueue::IncreaseFence() {
	
	std::lock_guard<std::mutex> lock(m_fence_mutex);
	m_command_queue->Signal(m_fence.Get(), m_next_fence_value);
	return m_next_fence_value++;

}

bool Fyuu::graphics::d3d12::CommandQueue::IsFenceCompleted(std::uint64_t fence_value) {

	if (fence_value > m_last_completed_fence_value) {
		m_last_completed_fence_value = std::max(m_last_completed_fence_value, m_fence->GetCompletedValue());
	}

	return  fence_value <= m_last_completed_fence_value;
	
}

void Fyuu::graphics::d3d12::CommandQueue::StallForFence(std::uint64_t fence_value) {

	auto& producer = Queue(static_cast<D3D12_COMMAND_LIST_TYPE>(fence_value >> 56));
	m_command_queue->Wait(producer->m_fence.Get(), fence_value);

}

void Fyuu::graphics::d3d12::CommandQueue::StallForProducer(CommandQueue const& producer) {

	m_command_queue->Wait(producer.m_fence.Get(), producer.m_next_fence_value);

}

void Fyuu::graphics::d3d12::CommandQueue::WaitForFence(std::uint64_t fence_value) {

	if (IsFenceCompleted(fence_value)) {
		return;
	}

	std::lock_guard<std::mutex> lock(m_fence_event_mutex);

	m_fence->SetEventOnCompletion(fence_value, m_fence_event);
	WaitForSingleObject(m_fence_event, INFINITE);
	m_last_completed_fence_value = fence_value;

}

std::uint64_t Fyuu::graphics::d3d12::CommandQueue::ExecuteCommandList(ID3D12CommandList* list) {

	std::lock_guard<std::mutex> lock(m_fence_mutex);
	ThrowIfFailed(reinterpret_cast<ID3D12GraphicsCommandList7*>(list)->Close());

	m_command_queue->ExecuteCommandLists(1, &list);
	m_command_queue->Signal(m_fence.Get(), m_next_fence_value);

	return m_next_fence_value++;

}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Fyuu::graphics::d3d12::CommandQueue::RequestAllocator() {

	return m_allocator_pool->RequestAllocator(m_fence->GetCompletedValue());

}

void Fyuu::graphics::d3d12::CommandQueue::DiscardAllocator(std::uint64_t fence_value_for_reset, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> const& allocator) {

	m_allocator_pool->DiscardAllocator(fence_value_for_reset, allocator);

}

std::shared_ptr<CommandQueue> const& Fyuu::graphics::d3d12::Queue(D3D12_COMMAND_LIST_TYPE type) {

	switch (type) {

	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return s_graphics_queue;

	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return s_compute_queue;

	case D3D12_COMMAND_LIST_TYPE_COPY:
		return s_copy_queue;

	default:
		return s_graphics_queue;

	}

}

std::shared_ptr<CommandQueue> const& Fyuu::graphics::d3d12::GraphicsQueue() noexcept {
	return s_graphics_queue;
}

std::shared_ptr<CommandQueue> const& Fyuu::graphics::d3d12::ComputeQueue() noexcept {
	return s_compute_queue;
}

std::shared_ptr<CommandQueue> const& Fyuu::graphics::d3d12::CopyQueue() noexcept {
	return s_copy_queue;
}

CommandObject Fyuu::graphics::d3d12::CreateNewCommandObject(D3D12_COMMAND_LIST_TYPE type) {

	CommandObject object;

	switch (type) {

	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		object.allocator = s_graphics_queue->RequestAllocator();
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		object.allocator = s_compute_queue->RequestAllocator();
		break;

	case D3D12_COMMAND_LIST_TYPE_COPY:
		object.allocator = s_copy_queue->RequestAllocator();
		break;

	default:
		throw std::runtime_error("Not a supported type");

	}

	ThrowIfFailed(D3D12Device()->CreateCommandList(1,type,object.allocator.Get(),nullptr,IID_PPV_ARGS(&object.list)));

	return object;

}

void Fyuu::graphics::d3d12::InitializeCommandListManager() {

	s_graphics_queue = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12Device());
	s_compute_queue = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COMPUTE, D3D12Device());
	s_copy_queue = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COPY, D3D12Device());

}

bool Fyuu::graphics::d3d12::IsFenceCompleted(std::uint64_t fence_value) {
	return Queue(static_cast<D3D12_COMMAND_LIST_TYPE>(fence_value >> 56))->IsFenceCompleted(fence_value);
}

void Fyuu::graphics::d3d12::WaitForFence(std::uint64_t fence_value) {

	auto& producer = Queue(static_cast<D3D12_COMMAND_LIST_TYPE>(fence_value >> 56));
	producer->WaitForFence(fence_value);

}

void Fyuu::graphics::d3d12::IdleGPU() {

	s_graphics_queue->WaitForIdle();
	s_compute_queue->WaitForIdle();
	s_copy_queue->WaitForIdle();

}
