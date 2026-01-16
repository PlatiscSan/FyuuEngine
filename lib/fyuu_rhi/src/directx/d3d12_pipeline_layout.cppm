module;
#ifdef WIN32
#include <dxgi1_6.h>
#include <directx/d3dx12.h>
#include <wrl.h>
#endif // WIN32

export module fyuu_rhi:d3d12_pipeline_layout;
import std;
import plastic.any_pointer;
import :pipeline;
import :d3d12_common;

namespace fyuu_rhi::d3d12::details {

	struct BindingKey {
		std::uint32_t binding;
		std::uint32_t space = 0;

		auto operator<=>(BindingKey const& other) const noexcept = default;
	};

	struct BindInfo {
		std::uint32_t slot;
		bool is_uav;
	};

}

namespace std {
	template <> 
	struct hash<fyuu_rhi::d3d12::details::BindingKey> {
		std::size_t operator()(fyuu_rhi::d3d12::details::BindingKey const& x) const;
	};
}

namespace fyuu_rhi::d3d12 {
	
	class D3D12PipelineLayout
		: public IPipelineLayout {
	private:
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root_signature;

	public:
		D3D12PipelineLayout(
			plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device,
			PipelineLayoutDescriptor const& descriptor
		);

		D3D12PipelineLayout(
			D3D12LogicalDevice const& logical_device,
			PipelineLayoutDescriptor const& descriptor
		);

	};

}