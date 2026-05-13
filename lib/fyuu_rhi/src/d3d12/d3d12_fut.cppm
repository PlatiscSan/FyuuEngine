module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <type_traits>
#include <limits>
#include <system_error>
#include <memory>
#include <mutex>
#include <format>
#endif // !defined(__cpp_lib_modules)
#if defined(_WIN32)
#include <dxgi1_3.h>
#include <d3d12.h>
#include <wrl.h>
#endif // defined(_WIN32)
module fyuu_rhi:d3d12_future;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_traits;
import :d3d12_utility;

namespace fyuu_rhi::d3d12 {

	Backend::Future Backend::GetFuture(Promise& promise) noexcept {
		return std::move(promise.last_fut);
	}

	//export class Future {
	//private:
	//	static constexpr std::size_t Capacity = (std::numeric_limits<std::uint16_t>::max)() - 1u;
	//	using EventQueue = boost::lockfree::queue<HANDLE, boost::lockfree::capacity<Capacity>>;


	//	static inline thread_local EventQueue* s_event_queue;
	//	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	//	std::uint64_t m_fence_val;
	//	HANDLE m_fence_event;

	//public:
	//	Future(Microsoft::WRL::ComPtr<ID3D12Fence> const& fence, std::uint64_t val)
	//		: m_fence(fence), m_fence_val(val),
	//		m_fence_event(
	//			[]() {
	//				static thread_local std::once_flag event_queue_init;
	//				static thread_local EventQueue event_queue;
	//				std::call_once(
	//					event_queue_init,
	//					[]() {
	//						s_event_queue = &event_queue;
	//						static thread_local boost::scope::defer_guard s_cleanup{
	//							[]() {
	//								HANDLE handle;
	//								while (event_queue.pop(handle)) {
	//									CloseHandle(handle);
	//								}
	//							}
	//						};
	//					}
	//				);

	//				HANDLE handle;
	//				if (!event_queue.pop(handle)) {
	//					handle = CreateEvent(nullptr, false, false, nullptr);
	//					if (!handle) {
	//						throw std::system_error(std::error_code(GetLastError(), std::system_category()), "Failed to create event");
	//					}
	//				}

	//				return handle;

	//			}()) {
	//		m_fence->SetEventOnCompletion(m_fence_val, m_fence_event);
	//	}

	//	~Future() noexcept {
	//		try {
	//			s_event_queue->push(m_fence_event);
	//		}
	//		catch (std::exception const& ex) {
	//			if (Warning) {
	//				Warning(std::format("~d3d12::Future(): {}", ex.what()));
	//			}
	//		}
	//		
	//	}

	//	bool IsReady() const noexcept {
	//		return m_fence->GetCompletedValue() >= m_fence_val;
	//	}

	//	void Wait() const {

	//		if (IsReady()) {
	//			return;
	//		}

	//		WaitForSingleObject(m_fence_event, (std::numeric_limits<DWORD>::max)());

	//	}
	//};

	//export struct Promise {
	//	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	//	std::atomic_uint64_t next_fence_val;
	//};

}
#endif // defined(_WIN32)
