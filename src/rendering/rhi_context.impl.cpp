module;
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string>
#include <string_view>
#endif // !defined(__cpp_lib_modules)
#include <boost/thread/tss.hpp>
module fyuu_engine:rhi_context_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :rhi_context;
import fyuu_rhi;

namespace fyuu_engine::rendering {

	RHIContext::RHIContext(fyuu_rhi::API api, fyuu_rhi::InitOptions const& init)
		: m_physical_device(api, init),
		m_logical_device(m_physical_device),
		m_graphics_queue(m_logical_device, fyuu_rhi::CommandObjectType::AllCommands, fyuu_rhi::QueuePriority::High),
		m_surface(m_physical_device, init.platform_handle),
		m_swap_chain(m_physical_device, m_logical_device, m_graphics_queue, m_surface, {}),
		m_command_buffer(std::make_unique<boost::thread_specific_ptr<fyuu_rhi::CommandBuffer>>()) {

	}

	fyuu_rhi::CommandBuffer* RHIContext::GetCommandBuffer() {
		if (!m_command_buffer->get()) {
			m_command_buffer->reset(
				new fyuu_rhi::CommandBuffer(
					m_logical_device, 
					fyuu_rhi::CommandObjectType::AllCommands
				)
			);
		}
		return m_command_buffer->get();
	}

	std::string_view RHIContext::GetAPIString() const noexcept {
		return m_physical_device.GetAPIString();
	}

	std::uint32_t RHIContext::GetVendorID() const noexcept {
		return m_physical_device.GetVendorID();
	}

	std::uint32_t RHIContext::GetID() const noexcept {
		return m_physical_device.GetID();
	}

	std::string RHIContext::GetDescription() const {
		return m_physical_device.GetDescription();
	}

	void RHIContext::Resize() {
		m_swap_chain.Resize();
	}

} // namespace fyuu_engine::rendering
