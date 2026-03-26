/* opengl_command_queue.cppm */
module;
#include <version>
#include <deque>
#if !defined(__cpp_lib_modules)
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "glad.hpp"
export module fyuu_rhi:opengl_command_queue;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :opengl_common;
import :opengl_future;

namespace fyuu_rhi::opengl {
	
	export class OpenGLCommandQueue 
		: public PolymorphicCommandQueueBase,
		public OpenGLCommon<OpenGLCommandQueue> {
	private:
		OpenGLPromise m_internal_promise;
		std::deque<GLuint> m_fbos;

	public:
		OpenGLCommandQueue(OpenGLLogicalDevice const& logical_device, 			
			CommandObjectType queue_type = CommandObjectType::AllCommands,
			QueuePriority priority = QueuePriority::High
		);

		void WaitUntilCompleted();

		void ExecuteCommand(OpenGLCommandBuffer& command_buffer, OpenGLPromise& promise, std::shared_ptr<OpenGLFuture> const& future = nullptr);

		std::shared_ptr<OpenGLFuture> ExecuteCommand(OpenGLCommandBuffer& command_buffer, std::shared_ptr<OpenGLFuture> const& future = nullptr);

		std::deque<GLuint> AcquirePendingPresent() noexcept;

	};

} // namespace fyuu_rhi::opengl

#endif