module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <memory>
#include <tuple>
#include <optional>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:swap_chain;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :physical_device;
import :logical_device;
import :command_queue;
import :surface;
import :future;
import :types;

namespace fyuu_rhi {

	export class SwapChain {
	private:
		UniqueSwapChain m_impl;
	
	public:
		SwapChain(
			PhysicalDevice const& physical_device, 
			LogicalDevice const& logical_device,
			CommandQueue const& present_queue, 
			Surface const& surface,
			SwapChainOption const& swap_chain_option
		);
		
		void Resize(std::optional<std::size_t> const& back_buffer_size = std::nullopt);

		std::tuple<PolymorphicResourceBase*, PolymorphicViewBase*, Future> GetNextFrame();

		void Present(Future const& future = {});

		void SetVSync(bool mode);

	};

}