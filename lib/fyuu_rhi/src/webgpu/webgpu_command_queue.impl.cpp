/* webgpu_command_queue.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <functional>
#include <memory>
#include <string_view>
#endif // !defined(__cpp_lib_modules)
#include "webgpu.hpp"
module fyuu_rhi:webgpu_command_queue_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :webgpu_command_queue;
import :webgpu_logical_device;
import :webgpu_future;
import :webgpu_common;
import :types;

namespace fyuu_rhi::webgpu {

	WebGPUCommandQueue::WebGPUCommandQueue(
		WebGPULogicalDevice const& logical_device,
		CommandObjectType /*queue_type*/,
		QueuePriority /*priority*/,
		std::string_view debug_name
	) : PolymorphicCommandQueueBase(this), 
		WebGPUCommon(this), 
		m_impl(logical_device.GetQueue()), 
		m_internal_promise(logical_device) {

		if (!debug_name.empty()) {

		}

	}

	void WebGPUCommandQueue::WaitUntilCompleted() {
		auto future = m_internal_promise.GetFuture();
		future->Wait();
	}

	void WebGPUCommandQueue::ExecuteCommand(WebGPUCommandBuffer& command_buffer, WebGPUPromise& promise, std::shared_ptr<WebGPUFuture> const& future) {
	
		if (future) {
			future->Wait();
		}

		//wgpu::CommandBuffer nativeCmdBuf = command_buffer.GetNative();

		//m_queue.Submit(1, &nativeCmdBuf);

		promise.CommandQueueSignal(this);

	}

	std::shared_ptr<WebGPUFuture> WebGPUCommandQueue::ExecuteCommand(
		WebGPUCommandBuffer& command_buffer,
		std::shared_ptr<WebGPUFuture> const& future
	) {
		ExecuteCommand(command_buffer, m_internal_promise, future);
		return m_internal_promise.GetFuture();
	}

	wgpu::Queue WebGPUCommandQueue::GetNative() const noexcept {
        return m_impl;
    }

} // namespace fyuu_rhi::webgpu
