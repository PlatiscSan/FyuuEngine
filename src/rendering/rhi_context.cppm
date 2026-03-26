module;
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string>
#include <string_view>
#endif // !defined(__cpp_lib_modules)
#include <boost/thread/tss.hpp>
export module fyuu_engine:rhi_context;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
export import fyuu_rhi;

namespace fyuu_engine::rendering {

	export class RHIContext {
	private:
		fyuu_rhi::PhysicalDevice m_physical_device;
		fyuu_rhi::LogicalDevice m_logical_device;
		fyuu_rhi::CommandQueue m_graphics_queue;
		fyuu_rhi::Surface m_surface;
		fyuu_rhi::SwapChain m_swap_chain;
		std::unique_ptr<boost::thread_specific_ptr<fyuu_rhi::CommandBuffer>> m_command_buffer;

	public:
		RHIContext(fyuu_rhi::API api, fyuu_rhi::InitOptions const& init);
		RHIContext(RHIContext&& other) noexcept = default;

		fyuu_rhi::CommandBuffer* GetCommandBuffer();

		std::string_view GetAPIString() const noexcept;

		std::uint32_t GetVendorID() const noexcept;

		std::uint32_t GetID() const noexcept;

		std::string GetDescription() const;

		void Resize();

	};
	
} // namespace fyuu_engine::rendering
