#ifndef GPU_RESOURCE_H
#define GPU_RESOURCE_H

#include "D3D12Core.h"

namespace Fyuu::graphics::d3d12 {

	class GPUResource {

	public:

		GPUResource() = default;
		GPUResource(Microsoft::WRL::ComPtr<ID3D12Resource2> const& resource, D3D12_RESOURCE_STATES current_state)
			:m_resource(resource), m_usage_state(current_state) {}

		virtual ~GPUResource() noexcept {

			m_resource = nullptr;
			m_gpu_virtual_address = D3D12_GPU_VIRTUAL_ADDRESS(0);
			++m_version_id;

		}

		Microsoft::WRL::ComPtr<ID3D12Resource2> const& GetResource() const noexcept { return m_resource; }
		D3D12_RESOURCE_STATES const& GetUsageState() const noexcept { return m_transitioning_state; }

	protected:

		Microsoft::WRL::ComPtr<ID3D12Resource2> m_resource = nullptr;
		D3D12_RESOURCE_STATES m_usage_state = D3D12_RESOURCE_STATE_COMMON;
		D3D12_RESOURCE_STATES m_transitioning_state = D3D12_RESOURCE_STATES(-1);

		D3D12_GPU_VIRTUAL_ADDRESS m_gpu_virtual_address = D3D12_GPU_VIRTUAL_ADDRESS(0);

		// Used to identify when a resource changes so descriptors can be copied etc.
		std::uint32_t m_version_id = 0;

	};


}

#endif // !GPU_RESOURCE_H
