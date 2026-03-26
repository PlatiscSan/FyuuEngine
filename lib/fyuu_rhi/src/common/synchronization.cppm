module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <memory>
#include <shared_mutex>
#include <condition_variable>
#include <thread>
#include <optional>
#endif // defined(__cpp_lib_modules)
export module fyuu_rhi:synchronization;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

namespace fyuu_rhi {
	export struct SynchronizationObject {
		std::shared_mutex mtx;
		std::condition_variable_any cv;
		std::optional<std::thread::id> owner_id;

		void Acquire();
		void Release();
		void ThrowIfNotOwned();

	};
}