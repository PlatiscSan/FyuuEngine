/* d3d12_utility.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <system_error>
#include <type_traits>
#include <memory>
#include <vector>
#include <memory_resource>
#endif // !defined(__cpp_lib_modules)
#if defined(_WIN32)
#include <Windows.h>
#include <d3d12.h>
#include <comdef.h>
#include <wil/resource.h>
#endif // defined(_WIN32)
#define BOOST_DISABLE_ASSERTS
#include <boost/locale.hpp>
export module fyuu_rhi:d3d12_utility;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

namespace {
	thread_local std::vector<wil::unique_event> s_events;
}

namespace fyuu_rhi::d3d12 {

	export struct ManagedEvent {
		wil::unique_event impl;
		~ManagedEvent() noexcept {
			s_events.emplace_back(std::move(impl));
		}
	};

	export void ThrowIfFailed(HRESULT result) {

		if (!FAILED(result)) {
			return;
		}

		_com_error error(result);

		std::string message = boost::locale::conv::utf_to_utf<char>(error.ErrorMessage());

		throw std::system_error(std::error_code(result, std::system_category()), message);
		
	}


	export std::shared_ptr<ManagedEvent> CreateManagedEvent() {

		constexpr std::size_t BUFFER_SIZE = 1024 * 1024; // 1 MB
		static std::array<std::byte, BUFFER_SIZE> buffer;
		static std::pmr::monotonic_buffer_resource s_buffer_resource(
			buffer.data(), buffer.size(),
			std::pmr::null_memory_resource()
		);
		static std::pmr::synchronized_pool_resource pool(&s_buffer_resource);

		std::pmr::polymorphic_allocator<ManagedEvent> alloc(&pool);

		if (s_events.empty()) {
			return std::allocate_shared<ManagedEvent>(alloc, wil::unique_event(wil::EventOptions::None));
		}

		wil::unique_event event = std::move(s_events.back());
		s_events.pop_back();
		event.ResetEvent();

		return std::allocate_shared<ManagedEvent>(alloc, std::move(event));

	}

}
#endif // defined(_WIN32)
