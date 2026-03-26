module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <shared_mutex>
#include <optional>
#include <variant>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include <D3D12MemAlloc.h>
export module fyuu_rhi:d3d12_resource;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :d3d12_common;

namespace fyuu_rhi::d3d12 {

	export class D3D12Resource
		: public PolymorphicResourceBase,
		public D3D12Common<D3D12Resource> {
	public:
		enum class ViewType : std::uint8_t {
			Unknown,
			CBV,
			UAV,
			SRV,
			RTV,
			DSV
		};

		enum class ResourceType {
			Unknown,
			Buffer,
			Texture
		};

	private:
		std::optional<std::size_t> m_logical_device_id;
		D3D12_RESOURCE_STATES m_state;

		std::variant<
			std::monostate, 
			Microsoft::WRL::ComPtr<D3D12MA::Allocation>, 
			Microsoft::WRL::ComPtr<ID3D12Resource2>
		> m_handle;

		ViewType m_view;
		ResourceType m_type;

	public:
		D3D12Resource(D3D12LogicalDevice const& logical_device, BufferSize buffer_size, VideoMemoryType type, ResourceFlags flags);

		D3D12Resource(D3D12LogicalDevice const& logical_device, TextureSize const& texture_size, VideoMemoryType type, ResourceFlags flags);

		D3D12Resource(D3D12LogicalDevice const& logical_device,IDXGISwapChain4* swap_chain, std::size_t index);

		D3D12Resource(D3D12Resource&& other) noexcept = default;

		D3D12_RESOURCE_DESC GetDescription() const noexcept;

		D3D12_RESOURCE_DESC1 GetDescription1() const noexcept;

		ID3D12Resource2* GetRawNative() const noexcept;

		D3D12_RESOURCE_STATES GetState() const;

		void SetState(D3D12_RESOURCE_STATES state) noexcept;

		ViewType GetViewType() const noexcept;

		ResourceType GetResourceType() const noexcept;

		std::size_t QuerySupportedMultiSampleQuality(std::size_t sample_count) const;

	};


}

#endif // defined(_WIN32)