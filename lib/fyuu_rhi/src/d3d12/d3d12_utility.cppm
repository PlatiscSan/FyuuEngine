/* d3d12_utility.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <system_error>
#include <type_traits>
#include <memory>
#include <vector>
#endif // !defined(__cpp_lib_modules)
#if defined(_WIN32)
#include <Windows.h>
#include <d3d12.h>
#include <comdef.h>
#include <wil/resource.h>
#endif // defined(_WIN32)
#define BOOST_DISABLE_ASSERTS
#include <boost/locale.hpp>
#include <boost/scope/unique_resource.hpp>
export module fyuu_rhi:d3d12_utility;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace fyuu_rhi::d3d12 {

	export void ThrowIfFailed(HRESULT result) {

		if (!FAILED(result)) {
			return;
		}

		_com_error error(result);

		std::string message = boost::locale::conv::utf_to_utf<char>(error.ErrorMessage());

		throw std::system_error(std::error_code(result, std::system_category()), message);
		
	}

	export using UniqueEvent = boost::scope::unique_resource<wil::unique_event, void(*)(wil::unique_event&)>;

	export using UniqueWait = wil::unique_any<HANDLE, decltype(&::UnregisterWait), ::UnregisterWait>;

	export UniqueEvent CreateEventUnique() {

		static thread_local std::vector<wil::unique_event> events;
		static auto GC = [](wil::unique_event& event) {
			events.emplace_back(std::move(event));
			};

		if (events.empty()) {
			return UniqueEvent(wil::unique_event(wil::EventOptions::None), GC);
		}

		wil::unique_event event = std::move(events.back());
		events.pop_back();
		event.ResetEvent();

		return UniqueEvent(std::move(event), GC);
	}

}
#endif // defined(_WIN32)
