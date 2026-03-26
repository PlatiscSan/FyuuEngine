module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // !defined(__cpp_lib_modules)
#include "webgpu.hpp"
module fyuu_rhi:webgpu_swap_chain_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :webgpu_swap_chain;
import :types;
import :webgpu_common;
import :webgpu_future;

namespace fyuu_rhi::webgpu {

    void WebGPUSwapChain::Resize(std::optional<std::size_t> const& back_buffer_size) {

    }

	std::tuple<WebGPUResource*, WebGPUView*, std::shared_ptr<WebGPUFuture>> WebGPUSwapChain::GetNextFrame() {
		return { nullptr, nullptr, nullptr };
	}

	void WebGPUSwapChain::Present(std::shared_ptr<WebGPUFuture> const& future) {

    }

	void WebGPUSwapChain::SetVSync(bool mode) {

    }

} // namespace fyuu_rhi::webgpu