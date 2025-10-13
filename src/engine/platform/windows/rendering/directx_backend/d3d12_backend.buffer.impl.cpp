module;
#ifdef WIN32
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <comdef.h>
#endif // WIN32

module d3d12_backend:buffer;
import :util;

namespace fyuu_engine::windows::d3d12 {
#ifdef WIN32
	D3D12Buffer::D3D12Buffer(Microsoft::WRL::ComPtr<ID3D12Resource> const& resource, std::optional<D3D12HeapChunk>&& chunk)
		: m_resource(resource), m_chunk(std::move(*chunk)) {
	}

	void D3D12Buffer::Update(std::span<std::byte> data) {

		D3D12_HEAP_PROPERTIES heap_prop{};
		D3D12_HEAP_FLAGS flag{};
		ThrowIfFailed(m_resource->GetHeapProperties(&heap_prop, &flag));

		void* map = nullptr;

		/*
		*	Do not intend to read from this resource on the CPU
		*/

		D3D12_RANGE read_range = { 0, 0 };

		switch (heap_prop.Type) {
		case D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD:
			ThrowIfFailed(m_resource->Map(0, &read_range, &map));
			break;
		default:
			throw std::runtime_error("Only upload buffer can be update directly");
		}

		std::memcpy(map, data.data(), data.size());
		m_resource->Unmap(0, nullptr);

	}

	D3D12_GPU_VIRTUAL_ADDRESS D3D12Buffer::GPUAddress() const noexcept {
		return m_resource->GetGPUVirtualAddress();
	}

	ID3D12Resource* D3D12Buffer::Native() const noexcept {
		return m_resource.Get();
	}

	std::size_t D3D12Buffer::Size() const noexcept {
		return m_chunk.size;
	}

	D3D12BufferCollector::D3D12BufferCollector(D3D12HeapPool* pool) noexcept
		: m_pool(pool) {
	}

	void D3D12BufferCollector::operator()(D3D12Buffer&& buffer) {
		if (m_pool) {
			m_pool->Free(buffer.m_chunk);
		}
	}

	void D3D12BufferCollector::operator()(D3D12Buffer& buffer) {
		if (m_pool) {
			m_pool->Free(buffer.m_chunk);
		}
	}
#endif // WIN32

}