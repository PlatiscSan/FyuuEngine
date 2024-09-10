#ifndef LINEAR_ALLOCATOR_H
#define LINEAR_ALLOCATOR_H

#include "GPUResource.h"

namespace Fyuu::graphics::d3d12 {

	class LinearAllocationPage : public GPUResource {

	public:

		LinearAllocationPage(Microsoft::WRL::ComPtr<ID3D12Resource2> const& resource, D3D12_RESOURCE_STATES usage)
			:GPUResource(resource, usage) {
			m_gpu_virtual_address = m_resource->GetGPUVirtualAddress();
			m_resource->Map(0, nullptr, &m_cpu_virtual_address);
		}

		~LinearAllocationPage() noexcept {

			if (m_cpu_virtual_address) {
				m_resource->Unmap(0, nullptr);
				m_cpu_virtual_address = nullptr;
			}

		}

	private:

		void* m_cpu_virtual_address;

	};

}

#endif // !LINEAR_ALLOCATOR_H
