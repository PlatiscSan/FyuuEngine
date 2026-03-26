/* command_queue.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string_view>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:command_queue;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

import :enums;
import :types;
import :logical_device;
import :future;

namespace fyuu_rhi {
    
	export class CommandQueue {
	private:
		UniqueCommandQueue m_impl;

	public:
		CommandQueue(LogicalDevice& logical_device, CommandObjectType type, QueuePriority priority);

		void WaitUntilCompleted();

		//Future ExecuteCommand(CommandBuffer& command_buffer, Promise& promise, Future const& future = {});

		PolymorphicCommandQueueBase* GetHandle() const noexcept;

		API GetAPI() const noexcept;

		std::string_view GetAPIString() const noexcept;

	};

} // namespace fyuu_rhi

