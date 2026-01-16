module;
#include <d3d12.h>
#include <wrl.h>
export module fyuu_rhi:d3d12_view;
import std;
import :d3d12_common;
import :view;

namespace fyuu_rhi::d3d12 {

	export class D3D12View
		: public IView {
	private:
		std::variant<std::monostate, D3D12_VERTEX_BUFFER_VIEW, D3D12_INDEX_BUFFER_VIEW> m_impl;

	public:
		D3D12View(D3D12LogicalDevice& logical_device, ViewType type);
	};

}