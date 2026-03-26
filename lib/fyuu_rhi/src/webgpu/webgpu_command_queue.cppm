/* webgpu_command_queue.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <string_view>
#include <memory>
#endif // !defined(__cpp_lib_modules)
#include "webgpu.hpp"
export module fyuu_rhi:webgpu_command_queue;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :webgpu_common;
import :webgpu_future;

namespace fyuu_rhi::webgpu {

	export class WebGPUCommandQueue
		: public PolymorphicCommandQueueBase,
		  public WebGPUCommon<WebGPUCommandQueue> {
	private:
		wgpu::Queue m_impl;
		WebGPUPromise m_internal_promise;

	public:
		WebGPUCommandQueue(
			WebGPULogicalDevice const& logical_device,
			CommandObjectType queue_type,
			QueuePriority priority = QueuePriority::High,
			std::string_view debug_name = {}
		);

		void WaitUntilCompleted();
		
		void ExecuteCommand(
			WebGPUCommandBuffer& command_buffer,
			WebGPUPromise& promise,
			std::shared_ptr<WebGPUFuture> const& future = nullptr
		);

		std::shared_ptr<WebGPUFuture> ExecuteCommand(
			WebGPUCommandBuffer& command_buffer,
			std::shared_ptr<WebGPUFuture> const& future = nullptr
		);

		wgpu::Queue GetNative() const noexcept;

	};

} // namespace fyuu_rhi::webgpu