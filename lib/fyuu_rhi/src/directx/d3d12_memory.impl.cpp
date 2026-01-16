module;
#include <D3D12MemAlloc.h>
#include <directx/d3dx12.h>
#include <wrl.h>
module fyuu_rhi:d3d12_memory;
import :d3d12_physical_device;
import :d3d12_logical_device;
import :d3d12_resource;

namespace fyuu_rhi::d3d12 {

	static D3D12_HEAP_TYPE VideoMemoryTypeToD3D12HeapType(VideoMemoryType type) noexcept {
		switch (type) {
		case fyuu_rhi::VideoMemoryType::DeviceReadback:
			return D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK;

		case fyuu_rhi::VideoMemoryType::HostVisible:
			return D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;

		case fyuu_rhi::VideoMemoryType::DeviceLocal:
		default:
			return D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		}
	}

	static D3D12_HEAP_FLAGS DetermineHeapFlagsFromUsage(VideoMemoryUsage usage) noexcept {
		switch (usage) {
		case fyuu_rhi::VideoMemoryUsage::Texture3D:
		case fyuu_rhi::VideoMemoryUsage::Texture2D:
			return D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_DENY_BUFFERS;

		case fyuu_rhi::VideoMemoryUsage::IndexBuffer:
		case fyuu_rhi::VideoMemoryUsage::VertexBuffer:
			return D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

		case fyuu_rhi::VideoMemoryUsage::Unknown:
		default:
			return D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		}
	}

	static Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocate(
		D3D12LogicalDevice const& logical_device,
		std::size_t size_in_bytes,
		VideoMemoryUsage usage,
		VideoMemoryType type
	) {

		Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation;
		D3D12_RESOURCE_DESC resource_desc = CD3DX12_RESOURCE_DESC::Buffer(size_in_bytes);

		D3D12_RESOURCE_ALLOCATION_INFO alloc_info = logical_device.GetRawNative()->GetResourceAllocationInfo(
			0,
			1,
			&resource_desc
		);

		D3D12MA::ALLOCATION_DESC desc{
			D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_STRATEGY_FIRST_FIT,
			VideoMemoryTypeToD3D12HeapType(type),
			DetermineHeapFlagsFromUsage(usage)
		};

		ThrowIfFailed(logical_device.GetRawVideoMemoryAllocator()->AllocateMemory(&desc, &alloc_info, &allocation));

		return allocation;

	}

	static D3D12_RESOURCE_DESC CreateResourceDesc(std::size_t width, std::size_t height, std::size_t depth, VideoMemoryUsage usage) noexcept {
		switch (usage) {
		case fyuu_rhi::VideoMemoryUsage::Texture3D:
			return CD3DX12_RESOURCE_DESC::Tex3D(DXGI_FORMAT::DXGI_FORMAT_UNKNOWN, width, static_cast<UINT>(height), static_cast<UINT16>(depth));
		case fyuu_rhi::VideoMemoryUsage::Texture2D:
			return CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT::DXGI_FORMAT_UNKNOWN, width, static_cast<UINT>(height));

		case fyuu_rhi::VideoMemoryUsage::IndexBuffer:
		case fyuu_rhi::VideoMemoryUsage::VertexBuffer:
		case fyuu_rhi::VideoMemoryUsage::Unknown:
		default:
			return CD3DX12_RESOURCE_DESC::Buffer(width * height, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE);
		}
	}

	static D3D12_RESOURCE_STATES DetermineInitialState(VideoMemoryUsage usage, VideoMemoryType type) {

		if (type == VideoMemoryType::HostVisible) {
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ;
		}

		return usage == VideoMemoryUsage::Unknown ?
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON :
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	}

	D3D12VideoMemory::UniqueResourceHandle D3D12VideoMemory::CreateBufferHandleImpl(std::size_t size_in_bytes) {

		if (m_allocated_flag.test_and_set(std::memory_order::acq_rel)) {
			throw AllocationError("The memory is bound already");
		}

		ID3D12Heap* heap = m_allocation->GetHeap();
		D3D12_HEAP_DESC heap_desc = heap->GetDesc();

		if (heap_desc.SizeInBytes < size_in_bytes) {
			throw AllocationError("Insufficient memory");
		}

		plastic::utility::AnyPointer<D3D12LogicalDevice> logical_device
			= plastic::utility::QueryObject<D3D12LogicalDevice>(m_device_id);

		D3D12_RESOURCE_DESC resource_desc = CreateResourceDesc(size_in_bytes, 1u, 1u, m_usage);
		D3D12_RESOURCE_STATES init_state = DetermineInitialState(m_usage, m_type);

		Microsoft::WRL::ComPtr<ID3D12Resource2> d3d12_resource;

		ID3D12Device2* d3d12_device = logical_device->GetRawNative();
		ThrowIfFailed(
			d3d12_device->CreatePlacedResource(
				heap,
				m_allocation->GetOffset(),
				&resource_desc,
				init_state,
				nullptr,
				IID_PPV_ARGS(&d3d12_resource)
			)
		);

		m_allocation->SetResource(d3d12_resource.Get());
		return UniqueResourceHandle(D3D12ResourceHandle{ d3d12_resource, init_state }, GetID());

	}

	D3D12VideoMemory::UniqueResourceHandle D3D12VideoMemory::CreateTextureHandleImpl(
		std::size_t width, 
		std::size_t height, 
		std::size_t depth
	) {
		return UniqueResourceHandle(D3D12ResourceHandle{}, 0);
	}

	VideoMemoryUsage D3D12VideoMemory::GetUsageImpl() const noexcept {
		return m_usage;
	}

	VideoMemoryType D3D12VideoMemory::GetTypeImpl() const noexcept {
		return m_type;
	}

	D3D12VideoMemory::D3D12VideoMemory(
		plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device, 
		std::size_t size_in_bytes, 
		VideoMemoryUsage usage,
		VideoMemoryType type
	) : m_allocation(Allocate(*logical_device, size_in_bytes, usage, type)),
		m_device_id(logical_device->GetID()),
		m_usage(usage),
		m_type(type) {
	}

	D3D12VideoMemory::D3D12VideoMemory(
		D3D12LogicalDevice const& logical_device,
		std::size_t size_in_bytes, 
		VideoMemoryUsage usage, 
		VideoMemoryType type
	) : m_allocation(Allocate(logical_device, size_in_bytes, usage, type)),
		m_device_id(logical_device.GetID()),
		m_usage(usage),
		m_type(type) {
	}

	D3D12VideoMemory::~D3D12VideoMemory() noexcept {

		std::size_t spin = 0;

		while (m_allocated_flag.test(std::memory_order::relaxed)) {
			if (++spin > 100u) {
				m_allocated_flag.wait(true, std::memory_order::relaxed);
			}
			else {
				std::this_thread::yield();
			}
		}

	}

	D3D12VideoMemory::HandleGC::HandleGC(std::size_t video_memory_id) noexcept
		: m_video_memory_id(video_memory_id) {
	}

	D3D12VideoMemory::HandleGC::HandleGC(HandleGC const& other) noexcept
		: m_video_memory_id(other.m_video_memory_id) {
	}

	D3D12VideoMemory::HandleGC::HandleGC(HandleGC&& other) noexcept
		: m_video_memory_id(std::exchange(other.m_video_memory_id, 0u)) {
	}

	D3D12VideoMemory::HandleGC& D3D12VideoMemory::HandleGC::operator=(HandleGC const& other) noexcept {
		if (this != &other) {
			m_video_memory_id = other.m_video_memory_id;
		}
		return *this;
	}

	D3D12VideoMemory::HandleGC& D3D12VideoMemory::HandleGC::operator=(HandleGC&& other) noexcept {
		if (this != &other) {
			m_video_memory_id = std::exchange(other.m_video_memory_id, 0u);
		}
		return *this;
	}

	void D3D12VideoMemory::HandleGC::operator()(D3D12ResourceHandle& resource) {

		plastic::utility::AnyPointer<D3D12VideoMemory>
			video_memory = plastic::utility::QueryObject<D3D12VideoMemory>(m_video_memory_id);

		video_memory->m_allocation->SetResource(nullptr);

		video_memory->m_allocated_flag.clear(std::memory_order::relaxed);
		video_memory->m_allocated_flag.notify_one();

	}

	std::size_t D3D12VideoMemory::HandleGC::GetVideoMemoryID() const noexcept {
		return m_video_memory_id;
	}

}