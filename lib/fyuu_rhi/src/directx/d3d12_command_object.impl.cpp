module;
#ifdef _WIN32
#include <d3d12.h>
#include <wrl/client.h>
#include <comdef.h>
#endif // _WIN32

module fyuu_rhi:d3d12_command_object;
import :d3d12_physical_device;
import :d3d12_logical_device;
import :d3d12_common;

namespace fyuu_rhi::d3d12 {
#ifdef _WIN32
	static D3D12_COMMAND_LIST_TYPE QueueTypeToD3D12CommandListType(CommandObjectType queue_type) {
		switch (queue_type) {
		case CommandObjectType::AllCommands:
			return D3D12_COMMAND_LIST_TYPE_DIRECT;

		case CommandObjectType::Compute:
			return D3D12_COMMAND_LIST_TYPE_COMPUTE;

		case CommandObjectType::Copy:
			return D3D12_COMMAND_LIST_TYPE_COPY;

		default:
			return D3D12_COMMAND_LIST_TYPE_DIRECT;
		}
	}

	static Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CreateSingleCommandList(
		Microsoft::WRL::ComPtr<ID3D12Device2> const& logical_device,
		D3D12_COMMAND_LIST_TYPE type
	) {
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
		ThrowIfFailed(logical_device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator)));
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list;
		ThrowIfFailed(logical_device->CreateCommandList(0, type, allocator.Get(), nullptr, IID_PPV_ARGS(&command_list)));

		// Associate the command allocator with the command list so that it can be
		// retrieved when the command list is executed.
		ThrowIfFailed(command_list->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), allocator.Get()));

		// It should be noted that when assigning a COM object to the private data of a ID3D12Object object using the ID3D12Object::SetPrivateDataInterface method, the internal reference counter of the assigned COM object is incremented. The ref counter of the assigned COM object is only decremented if either the owning ID3D12Object object is destroyed or the instance of the COM object with the same interface is replaced with another COM object of the same interface or a NULL pointer. 

		return command_list;
	}

	D3D12CommandBuffer::D3D12CommandBuffer(
		plastic::utility::AnyPointer<ID3D12GraphicsCommandList2> const& d3d12_command_buffer
	) : m_impl(plastic::utility::MakeReferred(d3d12_command_buffer)) {
	}

	D3D12CommandBuffer::D3D12CommandBuffer(
		plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device, 
		CommandObjectType buffer_type
	) : m_impl(CreateSingleCommandList(logical_device->GetNative(), QueueTypeToD3D12CommandListType(buffer_type))) {
	}

	D3D12CommandBuffer::D3D12CommandBuffer(
		plastic::utility::AnyPointer<ID3D12Device2> const& logical_device, 
		D3D12_COMMAND_LIST_TYPE type
	) : m_impl(CreateSingleCommandList(logical_device, type)) {
	}

	D3D12CommandBuffer::D3D12CommandBuffer(D3D12CommandBuffer&& other) noexcept
		: m_impl(std::move(other.m_impl)) {
	}

	plastic::utility::AnyPointer<ID3D12GraphicsCommandList2> D3D12CommandBuffer::GetNative() const noexcept {
		return m_impl;
	}

	ID3D12GraphicsCommandList2* D3D12CommandBuffer::GetRawNative() const noexcept {
		return m_impl.Get();
	}

	static Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(
		plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device,
		D3D12_COMMAND_LIST_TYPE allocator_type
	) {
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
		ThrowIfFailed(logical_device->GetRawNative()->CreateCommandAllocator(allocator_type, IID_PPV_ARGS(&allocator)));
		return allocator;
	}

	D3D12CommandAllocator::D3D12CommandAllocator(
		plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device,
		CommandObjectType allocator_type
	) : m_type(QueueTypeToD3D12CommandListType(allocator_type)),
		m_impl(CreateCommandAllocator(logical_device, m_type)) {
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list;
		for (std::size_t i = 0; i < m_free_command_buffers.capacity(); ++i) {
			ThrowIfFailed(logical_device->GetRawNative()->CreateCommandList(0, m_type, m_impl.Get(), nullptr, IID_PPV_ARGS(&command_list)));
			m_free_command_buffers.emplace_back(std::move(command_list));
		}

	}

	D3D12CommandAllocator::UniqueCommandBuffer D3D12CommandAllocator::Allocate() {
		return UniqueCommandBuffer(*m_free_command_buffers.pop_front(true), GetID());
	}

	D3D12CommandAllocator::CommandBufferGC::CommandBufferGC(std::size_t allocator_id) noexcept
		: m_allocator_id(allocator_id) {
	}

	D3D12CommandAllocator::CommandBufferGC::CommandBufferGC(CommandBufferGC const& other) noexcept
		: m_allocator_id(other.m_allocator_id) {
	}

	D3D12CommandAllocator::CommandBufferGC::CommandBufferGC(CommandBufferGC&& other) noexcept
		: m_allocator_id(std::exchange(other.m_allocator_id, 0ull)) {
	}

	void D3D12CommandAllocator::CommandBufferGC::operator()(D3D12CommandBuffer& command_buffer) noexcept {

		plastic::utility::AnyPointer<D3D12CommandAllocator> allocator =
			plastic::utility::QueryObject<D3D12CommandAllocator>(m_allocator_id);

		allocator->m_free_command_buffers.emplace_back(std::move(command_buffer));

	}

	void D3D12CommandQueue::WaitUntilCompletedImpl() {
		Flush();
	}

	std::uint64_t D3D12CommandQueue::ExecuteCommandImpl(D3D12CommandBuffer& command_buffer) {

		ID3D12CommandAllocator* command_allocator;
		UINT data_size = sizeof(command_allocator);
		ThrowIfFailed(command_buffer.GetRawNative()->GetPrivateData(__uuidof(ID3D12CommandAllocator), &data_size, &command_allocator));
		// You should be aware that retrieving a COM pointer of a COM object associated with the private data of the ID3D12Object object will also increment the reference counter of that COM object.

		ID3D12CommandList* const command_lists[] = {
			command_buffer.GetRawNative()
		};

		m_impl->ExecuteCommandLists(std::size(command_lists), command_lists);
		std::uint64_t fence_value = Signal();


		// The ownership of the command allocator has been transferred to the ComPtr
		// in the command allocator queue. It is safe to release the reference 
		// in this temporary COM pointer here.
		command_allocator->Release();

		return fence_value;

	}

	static Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(
		D3D12LogicalDevice const& logical_device,
		D3D12_COMMAND_LIST_TYPE type
	) {
		
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> impl;

		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = type;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		ThrowIfFailed(logical_device.GetRawNative()->CreateCommandQueue(&desc, IID_PPV_ARGS(&impl)));

		return impl;

	}

	static Microsoft::WRL::ComPtr<ID3D12Fence> CreateFence(
		D3D12LogicalDevice const& logical_device
	) {
		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		ThrowIfFailed(logical_device.GetRawNative()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
		return fence;
	}

	D3D12CommandQueue::D3D12CommandQueue(
		plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device, 
		CommandObjectType queue_type,
		QueuePriority priority
	) : m_logical_device_id(logical_device->GetID()),
		m_command_list_type(QueueTypeToD3D12CommandListType(queue_type)),
		m_fence_value(0ull),
		m_impl(CreateCommandQueue(*logical_device, m_command_list_type)),
		m_fence(CreateFence(*logical_device)),
		m_fence_event(CreateEventHandle()) {

	}

	D3D12CommandQueue::D3D12CommandQueue(
		D3D12LogicalDevice const& logical_device,
		CommandObjectType queue_type,
		QueuePriority priority
	) : m_logical_device_id(logical_device.GetID()),
		m_command_list_type(QueueTypeToD3D12CommandListType(queue_type)),
		m_fence_value(0ull),
		m_impl(CreateCommandQueue(logical_device, m_command_list_type)),
		m_fence(CreateFence(logical_device)),
		m_fence_event(CreateEventHandle()) {
	}

	D3D12_COMMAND_LIST_TYPE D3D12CommandQueue::GetType() const noexcept {
		return m_command_list_type;
	}

	std::uint64_t D3D12CommandQueue::Signal() {
		std::uint64_t fence_value = ++m_fence_value;
		m_impl->Signal(m_fence.Get(), fence_value);
		return fence_value;
	}

	bool D3D12CommandQueue::IsComplete(std::uint64_t fence_value) {
		return m_fence->GetCompletedValue() >= fence_value;
	}

	void D3D12CommandQueue::Wait(std::uint64_t fence_value) {
		if (!IsComplete(fence_value)) {
			m_fence->SetEventOnCompletion(fence_value, m_fence_event.get());
			WaitForSingleObject(m_fence_event.get(), (std::numeric_limits<DWORD>::max)());
		}
	}

	void D3D12CommandQueue::Flush() {
		Wait(Signal());
	}

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> D3D12CommandQueue::GetNative() const noexcept {
		return m_impl;
	}

	ID3D12CommandQueue* D3D12CommandQueue::GetRawNative() const noexcept {
		return m_impl.Get();
	}


#endif // _WIN32

}