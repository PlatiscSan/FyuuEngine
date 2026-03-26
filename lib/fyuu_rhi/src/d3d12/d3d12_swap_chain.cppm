/* d3d12_swap_chain.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <tuple>
#include <vector>
#include <optional>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
export module fyuu_rhi:d3d12_swap_chain;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :d3d12_future;
import :d3d12_common;
import :d3d12_resource;
import :d3d12_view;

namespace fyuu_rhi::d3d12 {

	export class D3D12SwapChain 
		: public PolymorphicSwapChainBase,
		public D3D12Common<D3D12SwapChain> {
	private:
		struct Frame {
			D3D12Resource texture;
			D3D12View view;
			std::shared_ptr<D3D12Future> future;

			Frame(D3D12LogicalDevice const& logical_device, D3D12SwapChain const& swap_chain, std::size_t index);
		};

		Microsoft::WRL::ComPtr<IDXGISwapChain4> m_impl;
		D3D12Promise m_internal_promise;
		std::optional<std::size_t> m_logical_device_id;
		std::optional<std::size_t> m_present_queue_id;
		std::optional<std::size_t> m_surface_id;
		SwapChainOption m_option;
		
		std::vector<Frame> m_frames;

		void Resize(std::size_t width, std::size_t height, std::size_t back_buffer_size);

	public:
		D3D12SwapChain(
			D3D12PhysicalDevice const& physical_device,
			D3D12LogicalDevice const& logical_device,
			D3D12CommandQueue const& present_queue,
			D3D12Surface const& surface,
			SwapChainOption const& swap_chain_option
		);

		~D3D12SwapChain() noexcept;

		void Resize(std::optional<std::size_t> const& back_buffer_size = std::nullopt);

		std::tuple<D3D12Resource*, D3D12View*, std::shared_ptr<D3D12Future>> GetNextFrame();

		void Present(std::shared_ptr<D3D12Future> const& future = nullptr);

		void SetVSync(bool mode);

		IDXGISwapChain4* GetRawNative() const noexcept;

	};

}

#endif // defined(_WIN32)