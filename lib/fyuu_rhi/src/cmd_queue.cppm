module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <vector>
#endif // !defined(__cpp_lib_modules)

export module fyuu_rhi:command_queue;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :future;
import :resource;
import :view;

namespace fyuu_rhi {

	export template <class Backend> class SwapChain;

	export template <class Backend> class CommandQueue {
	public:
		using Implementation = typename Backend::CommandQueue;

	private:
		Implementation m_impl;

	public:
		template <std::convertible_to<Implementation> I>
		CommandQueue(I&& impl)
			: m_impl(std::forward<I>(impl)) {

		}

		void WaitUntilCompleted();

		void ExecuteCommand(CommandBuffer& command_buffer, Promise& promise, Future<Backend> const& future = {});

		Future<Backend> ExecuteCommand(CommandBuffer& command_buffer, Future<Backend> const& future = {});

		Future<Backend> ExecuteCommand(CommandBuffer& command_buffer, SwapChain<Backend>& present_target, Future<Backend> const& future = {});

	};

}