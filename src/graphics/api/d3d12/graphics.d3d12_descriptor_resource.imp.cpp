module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef WIN32
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <comdef.h>
#include "d3dx12.h"
#endif // WIN32

#ifdef interface
#undef interface
#endif // interface

module graphics:d3d12_descriptor_resource;
import std;

namespace graphics::api::d3d12 {
#ifdef WIN32

	static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		std::uint32_t count
	) {

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
		heap_desc.NumDescriptors = count;
		heap_desc.Type = type;
		heap_desc.Flags =
			(type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) ?
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE :
			D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		ThrowIfFailed(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap)));

		return heap;

	}

	static std::deque<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>>
		GetResourceDescriptorHandles(
			Microsoft::WRL::ComPtr<ID3D12Device> const& device,
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> const& heap_descriptor
		) {

		D3D12_DESCRIPTOR_HEAP_DESC desc = heap_descriptor->GetDesc();
		std::deque<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>> handles;
		std::uint32_t increment = device->GetDescriptorHandleIncrementSize(desc.Type);
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = heap_descriptor->GetCPUDescriptorHandleForHeapStart();

		D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = { 0 };
		bool is_shader_visible = (desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) != 0;

		if (is_shader_visible) {
			gpu_handle = heap_descriptor->GetGPUDescriptorHandleForHeapStart();
		}

		for (std::uint32_t i = 0; i < desc.NumDescriptors; ++i) {
			handles.emplace_back(cpu_handle, gpu_handle);
			cpu_handle.ptr += increment;
			if (is_shader_visible) {
				gpu_handle.ptr += increment;
			}
		}

		return handles;

	}

	std::unique_lock<std::mutex> DescriptorResourcePool::WaitForAllocated(DescriptorResourcePool* pool) {
		std::unique_lock<std::mutex> lock(pool->m_descriptor_handles_mutex);
		if (pool->m_descriptor_handles.size() != pool->m_count) {
			pool->m_condition.wait(
				lock,
				[pool]() {
					return pool->m_descriptor_handles.size() == pool->m_count;
				}
			);
		}
		return lock;
	}

	DescriptorResourcePool::DescriptorResourcePool(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		D3D12_DESCRIPTOR_HEAP_TYPE type, 
		std::uint32_t count
	) : m_descriptor_heap(CreateDescriptorHeap(device, type, count)),
		m_descriptor_handles(GetResourceDescriptorHandles(device, m_descriptor_heap)),
		m_count(count) {
	}

	DescriptorResourcePool::DescriptorResourcePool(DescriptorResourcePool&& other) noexcept
		: m_descriptor_heap(std::move(other.m_descriptor_heap)),
		m_descriptor_handles(std::move(other.m_descriptor_handles)),
		m_count(std::exchange(other.m_count, 0)) {
	}

	DescriptorResourcePool& DescriptorResourcePool::operator=(DescriptorResourcePool&& other) noexcept {
		if (this != &other) {
			auto lock = DescriptorResourcePool::WaitForAllocated(this);
			auto other_lock = DescriptorResourcePool::WaitForAllocated(&other);
			m_descriptor_heap = std::move(other.m_descriptor_heap);
			m_descriptor_handles = std::move(other.m_descriptor_handles);
			m_count = std::exchange(other.m_count, 0);
		}
		return *this;
	}

	DescriptorResourcePool::~DescriptorResourcePool() noexcept {
		auto lock = DescriptorResourcePool::WaitForAllocated(this);
		m_descriptor_handles.clear();
	}

	D3D12_DESCRIPTOR_HEAP_TYPE DescriptorResourcePool::PoolType() const noexcept {
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = m_descriptor_heap->GetDesc();
		return heap_desc.Type;
	}

	DescriptorResourcePool::UniqueDescriptorHandle DescriptorResourcePool::AcquireHandle() {
		std::unique_lock<std::mutex> lock(m_descriptor_handles_mutex);
		if (m_descriptor_handles.empty()) {
			m_condition.wait(
				lock, 
				[this]() {
					return !m_descriptor_handles.empty();
				}
			);
		}
		auto front = std::move(m_descriptor_handles.front());
		m_descriptor_handles.pop_front();
		return UniqueDescriptorHandle(front, Collector(this));
	}

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorResourcePool::GetDescriptorHeap() const noexcept {
		return m_descriptor_heap;
	}

	DescriptorResourcePool::Collector::Collector(DescriptorResourcePool* owner) noexcept
		: m_owner(owner) {
	}

	void DescriptorResourcePool::Collector::operator()(
		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> handle
		) {

		std::unique_lock<std::mutex> lock(m_owner->m_descriptor_handles_mutex);
		m_owner->m_descriptor_handles.emplace_back(std::move(handle));
		m_owner->m_condition.notify_one();

	}
#endif // WIN32
}