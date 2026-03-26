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
module fyuu_rhi:synchronization_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :synchronization;

namespace fyuu_rhi {

	void SynchronizationObject::Acquire() {
		std::unique_lock<std::shared_mutex> lock(mtx);
		std::thread::id this_id = std::this_thread::get_id();
	
		if (owner_id && *owner_id == this_id) {
			return;
		}

		cv.wait(lock, [this] { return !owner_id; });

		try {
			owner_id.emplace(std::this_thread::get_id());
		}
		catch(...) {
			cv.notify_one();
			throw;
		}
	}

	void SynchronizationObject::Release() {
		{
			std::lock_guard<std::shared_mutex> lock(mtx);
			if (owner_id && std::this_thread::get_id() != *owner_id) {
				throw std::runtime_error("Only the owner can release the resource");
			}
			owner_id.reset();
		}
		cv.notify_one();
	}

	void SynchronizationObject::ThrowIfNotOwned() {
		std::shared_lock<std::shared_mutex> lock(mtx);
		if (!owner_id || std::this_thread::get_id() != *owner_id) {
			throw std::runtime_error("Not owned by this thread");
		}
	}	

}