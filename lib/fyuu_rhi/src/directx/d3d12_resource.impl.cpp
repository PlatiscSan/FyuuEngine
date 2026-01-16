module;
#include <directx/d3dx12.h>
#include <wrl.h>
module fyuu_rhi:d3d12_resource;
import :d3d12_physical_device;
import :d3d12_logical_device;
import :d3d12_command_object;

namespace fyuu_rhi::d3d12 {

	void D3D12Resource::ReleaseImpl() noexcept {
		m_handle.Reset();
	}

	void D3D12Resource::SetMemoryHandleImpl(UniqueD3D12ResourceHandle&& handle) {
		m_handle = std::move(handle);
	}

	D3D12Resource::UniqueMappedHandle D3D12Resource::MapImpl(std::uintptr_t begin, std::uintptr_t end) {
		D3D12_RANGE map_range{ begin, end };
		void* ptr;
		m_handle->impl->Map(0, &map_range, &ptr);
		return UniqueMappedHandle(MappedHandle{ map_range, ptr }, GetID());
	}

	void D3D12Resource::SetDeviceLocalBuffer(
		D3D12LogicalDevice& logical_device,
		D3D12CommandQueue& copy_queue,
		std::span<std::byte const> raw,
		std::size_t offset
	) {

		D3D12VideoMemory upload_memory(
			&logical_device, 
			raw.size(), 
			DetermineMemoryUsageFromResourceType(m_type),
			VideoMemoryType::HostVisible
		);

		D3D12Resource upload_resource(&upload_memory, raw.size(), 1u, 1u, m_type);
		upload_resource.SetBufferDataImpl(logical_device, copy_queue, raw, offset);

		D3D12CommandBuffer command_buffer(logical_device.GetNative(), copy_queue.GetType());

		if (m_handle->state != D3D12_RESOURCE_STATE_COPY_DEST) {
			D3D12_RESOURCE_BARRIER begin_transition = CD3DX12_RESOURCE_BARRIER::Transition(
				m_handle->impl.Get(),
				m_handle->state,
				D3D12_RESOURCE_STATE_COPY_DEST
			);

			command_buffer.GetRawNative()->ResourceBarrier(1, &begin_transition);
		}

		command_buffer.GetRawNative()->CopyBufferRegion(m_handle->impl.Get(), offset, upload_resource.m_handle->impl.Get(), 0, raw.size());

		switch (m_type) {
		case ResourceType::VertexBuffer:
			m_handle->state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			break;
		case ResourceType::IndexBuffer:
			m_handle->state = D3D12_RESOURCE_STATE_INDEX_BUFFER;
			break;
		default:
			break;
		}

		D3D12_RESOURCE_BARRIER end_transition = CD3DX12_RESOURCE_BARRIER::Transition(
			m_handle->impl.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			m_handle->state
		);
		command_buffer.GetRawNative()->ResourceBarrier(1, &end_transition);

		command_buffer.GetRawNative()->Close();

		std::uint32_t fence_value = copy_queue.ExecuteCommand(command_buffer);
		copy_queue.Wait(fence_value);

	}

	void D3D12Resource::SetHostVisibleBuffer(std::span<std::byte const> raw, std::size_t offset) {
		UniqueMappedHandle mapped_handle = MapImpl(0u, raw.size());
		std::memcpy(mapped_handle->ptr, raw.data(), raw.size());
	}

	void D3D12Resource::SetBufferDataImpl(
		D3D12LogicalDevice& logical_device,
		D3D12CommandQueue& copy_queue,
		std::span<std::byte const> raw,
		std::size_t offset
	) {

		switch (m_type) {
		case ResourceType::VertexBuffer:
		case ResourceType::IndexBuffer:
			break;

		default:
			throw std::runtime_error("Not buffer");
		}

		plastic::utility::AnyPointer<D3D12VideoMemory> video_memory
			= plastic::utility::QueryObject<D3D12VideoMemory>(m_handle.GetGC().GetVideoMemoryID());

		VideoMemoryType mem_type = video_memory->GetType();

		switch (mem_type) {
		case fyuu_rhi::VideoMemoryType::DeviceReadback:
			break;

		case fyuu_rhi::VideoMemoryType::HostVisible:
			SetHostVisibleBuffer(raw, offset);
			break;

		case fyuu_rhi::VideoMemoryType::DeviceLocal:
		default:
			SetDeviceLocalBuffer(logical_device, copy_queue, raw, offset);
			break;
		}

	}

	D3D12Resource D3D12Resource::CreateVertexBuffer(std::size_t size_in_bytes) {
		return D3D12Resource(size_in_bytes, 1, 1, ResourceType::VertexBuffer);
	}

	D3D12Resource D3D12Resource::CreateIndexBuffer(std::size_t size_in_bytes) {
		return D3D12Resource(size_in_bytes, 1, 1, ResourceType::IndexBuffer);
	}

	D3D12Resource::D3D12Resource(std::size_t width, std::size_t height, std::size_t depth, ResourceType type)
		: m_width(width),
		m_height(height),
		m_depth(depth),
		m_type(type),
		m_handle(D3D12ResourceHandle{}, 0) {
	}

	static UniqueD3D12ResourceHandle CreateResourceHandleFromMemory(
		D3D12VideoMemory& memory,
		std::size_t width,
		std::size_t height,
		std::size_t depth,
		ResourceType type
	) {
		if (DetermineResourceTypeFromMemoryUsage(memory.GetUsage()) == ResourceType::Unknown) {
			throw std::runtime_error("Incompatible memory");
		}

		switch (type) {
		case ResourceType::VertexBuffer:
		case ResourceType::IndexBuffer:
			return memory.CreateBufferHandle(width);

		case ResourceType::Texture1D:
		case ResourceType::Texture2D:
		case ResourceType::Texture3D:
			return memory.CreateTextureHandle(width, height, depth);

		default:
			throw std::invalid_argument("Invalid resource type");
		}
	}

	D3D12Resource::D3D12Resource(
		plastic::utility::AnyPointer<D3D12VideoMemory> const& memory,
		std::size_t width,
		std::size_t height,
		std::size_t depth,
		ResourceType type
	) : m_width(width),
		m_height(height),
		m_depth(depth),
		m_type(type),
		m_handle(CreateResourceHandleFromMemory(*memory, width, height, depth, type)) {
	}

	D3D12Resource::D3D12Resource(
		D3D12VideoMemory& memory, 
		std::size_t width, 
		std::size_t height, 
		std::size_t depth, 
		ResourceType type
	) : m_width(width),
		m_height(height),
		m_depth(depth),
		m_type(type),
		m_handle(CreateResourceHandleFromMemory(memory, width, height, depth, type)) {
	}

	D3D12Resource::~D3D12Resource() noexcept {
		ReleaseImpl();
	}

	D3D12Resource::MappedHandleGC::MappedHandleGC(std::size_t resource_id) noexcept
		: m_resource_id(resource_id) {
	}

	D3D12Resource::MappedHandleGC::MappedHandleGC(MappedHandleGC const& other) noexcept
		: m_resource_id(other.m_resource_id) {
	}

	D3D12Resource::MappedHandleGC::MappedHandleGC(MappedHandleGC&& other) noexcept
		: m_resource_id(std::exchange(other.m_resource_id, 0u)) {
	}

	D3D12Resource::MappedHandleGC& D3D12Resource::MappedHandleGC::operator=(MappedHandleGC const& other) noexcept {
		if (this != &other) {
			m_resource_id = other.m_resource_id;
		}
		return *this;
	}

	D3D12Resource::MappedHandleGC& D3D12Resource::MappedHandleGC::operator=(MappedHandleGC&& other) noexcept {
		if (this != &other) {
			m_resource_id = std::exchange(other.m_resource_id, 0u);
		}
		return *this;
	}

	void D3D12Resource::MappedHandleGC::operator()(MappedHandle& handle) {

		plastic::utility::AnyPointer<D3D12Resource>
			resource = plastic::utility::QueryObject<D3D12Resource>(m_resource_id);

		resource->m_handle->impl->Unmap(0, &handle.range);

	}

	std::size_t D3D12Resource::MappedHandleGC::GetResourceID() const noexcept {
		return m_resource_id;
	}

}