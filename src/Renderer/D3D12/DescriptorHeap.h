#ifndef DESCRIPTOR_HEAP_H
#define DESCRIPTOR_HEAP_H

#include "D3D12Core.h"

namespace Fyuu::graphics::d3d12 {

	class DescriptorAllocator {

	public:

		DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) 
			:m_type(type), m_descriptor_size(0), m_remaining_free_handles(0) {
			m_current_handle.ptr = D3D12_GPU_VIRTUAL_ADDRESS(-1);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Allocate(std::size_t count);


	protected:

		D3D12_DESCRIPTOR_HEAP_TYPE m_type;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_current_heap;
		D3D12_CPU_DESCRIPTOR_HANDLE m_current_handle;
		std::size_t m_descriptor_size;
		std::size_t m_remaining_free_handles;

	};

	void DestroyDescriptorAllocators();
	D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, std::size_t count = 1);

	class DescriptorHandle final {

	public:

		DescriptorHandle() {
			m_cpu_handle.ptr = D3D12_GPU_VIRTUAL_ADDRESS(-1);
			m_gpu_handle.ptr = D3D12_GPU_VIRTUAL_ADDRESS(-1);
		}

		DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
			:m_cpu_handle(cpu_handle), m_gpu_handle(gpu_handle) {}

		DescriptorHandle operator+(std::int32_t offset_scaled_by_descriptor_size) const noexcept {

			DescriptorHandle ret = *this;

			if (ret.m_cpu_handle.ptr != D3D12_GPU_VIRTUAL_ADDRESS(-1)) {
				ret.m_cpu_handle.ptr += offset_scaled_by_descriptor_size;
			}

			if (ret.m_gpu_handle.ptr != D3D12_GPU_VIRTUAL_ADDRESS(-1)) {
				ret.m_gpu_handle.ptr += offset_scaled_by_descriptor_size;
			}

			return ret;

		}

		DescriptorHandle operator*(std::int32_t offset_scaled_by_descriptor_size) const noexcept {

			DescriptorHandle ret = *this;

			if (ret.m_cpu_handle.ptr != D3D12_GPU_VIRTUAL_ADDRESS(-1)) {
				ret.m_cpu_handle.ptr *= offset_scaled_by_descriptor_size;
			}

			if (ret.m_gpu_handle.ptr != D3D12_GPU_VIRTUAL_ADDRESS(-1)) {
				ret.m_gpu_handle.ptr *= offset_scaled_by_descriptor_size;
			}

			return ret;

		}

		DescriptorHandle operator+(DescriptorHandle const& other) const noexcept {

			DescriptorHandle ret = *this;

			if (ret.m_cpu_handle.ptr != D3D12_GPU_VIRTUAL_ADDRESS(-1)) {
				ret.m_cpu_handle.ptr += other.m_cpu_handle.ptr;
			}

			if (ret.m_gpu_handle.ptr != D3D12_GPU_VIRTUAL_ADDRESS(-1)) {
				ret.m_gpu_handle.ptr += other.m_gpu_handle.ptr;
			}

			return ret;

		}

		DescriptorHandle& operator+=(std::int32_t offset_scaled_by_descriptor_size) noexcept {

			if (m_cpu_handle.ptr != D3D12_GPU_VIRTUAL_ADDRESS(-1)) {
				m_cpu_handle.ptr += offset_scaled_by_descriptor_size;
			}

			if (m_gpu_handle.ptr != D3D12_GPU_VIRTUAL_ADDRESS(-1)) {
				m_gpu_handle.ptr += offset_scaled_by_descriptor_size;
			}

			return *this;

		}

		DescriptorHandle& operator-=(std::int32_t offset_scaled_by_descriptor_size) noexcept {

			if (m_cpu_handle.ptr != D3D12_GPU_VIRTUAL_ADDRESS(-1)) {
				m_cpu_handle.ptr -= offset_scaled_by_descriptor_size;
			}

			if (m_gpu_handle.ptr != D3D12_GPU_VIRTUAL_ADDRESS(-1)) {
				m_gpu_handle.ptr -= offset_scaled_by_descriptor_size;
			}

			return *this;

		}

		D3D12_CPU_DESCRIPTOR_HANDLE const& operator&() const { return m_cpu_handle; }
		operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return m_cpu_handle; }
		operator D3D12_GPU_DESCRIPTOR_HANDLE() const { return m_gpu_handle; }

		std::size_t const& GetCPUPtr() const noexcept { return m_cpu_handle.ptr; }
		std::size_t const& GetGPUPtr() const noexcept { return m_gpu_handle.ptr; }
		bool IsNull() const noexcept { return m_cpu_handle.ptr == D3D12_GPU_VIRTUAL_ADDRESS(0); }
		bool IsShaderVisible() const noexcept { return m_gpu_handle.ptr != D3D12_GPU_VIRTUAL_ADDRESS(0); }



	private:

		D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_handle;
		D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_handle;

	};

	class DescriptorHeap final {

	public:

		DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, std::uint32_t max_count);
		~DescriptorHeap() noexcept = default;

		bool HasAvailableSpace(std::uint32_t count) const { return count <= m_num_free_descriptors; }
		DescriptorHandle Allocate(std::uint32_t count = 1);
		DescriptorHandle operator[] (std::uint32_t index) const noexcept { return m_first_handle + (m_next_free_handle * index); }

		std::uint32_t GetOffsetOfHandle(DescriptorHandle const& handle) {
			return (std::uint32_t)(handle.GetCPUPtr() - m_first_handle.GetGPUPtr()) / m_descriptor_size;
		}

		bool ValidateHandle(DescriptorHandle const& handle) const noexcept;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> const& GetHeapPointer() const noexcept { return m_heap; }

		std::uint32_t const& GetDescriptorSize() const noexcept { return m_descriptor_size; }

	private:

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;
		D3D12_DESCRIPTOR_HEAP_DESC m_heap_desc;
		std::uint32_t m_descriptor_size;
		std::size_t m_num_free_descriptors;
		DescriptorHandle m_first_handle;
		DescriptorHandle m_next_free_handle;

	};

}

#endif // !DESCRIPTOR_HEAP_H
