module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <type_traits>
#include <concepts>
#endif
export module fyuu_rhi:future;
#if defined(__cpp_lib_modules)
import std;              // Import the entire standard library if module support is available.
// In non‑module builds, the required headers were already included.
#endif
import :types;
import :logical_device;

#if !defined(__APPLE__)
import :vulkan_future;
import :opengl_future;
#endif // !defined(__APPLE__)
#if defined(_WIN32)
import :d3d12_future;
#endif // defined(_WIN32)
#if defined(__APPLE__)
import :metal_future;
#endif // defined(__APPLE__)
import :webgpu_future;

namespace fyuu_rhi {

	export class Future {
	private:
		std::shared_ptr<PolymorphicFutureBase> m_impl;

	public:
		Future() = default;
		Future(std::shared_ptr<PolymorphicFutureBase> const& impl) noexcept;

		template <std::derived_from<PolymorphicFutureBase> Derived>
		Future(std::shared_ptr<Derived> const& impl) noexcept
			: m_impl(impl) {

		}

		void Wait();

		template <class Value>
		Value GetValue() {
			m_impl->Visit(
				[](auto* derived) -> Value {
					return derived->GetValue();
				}
			);
		}

		std::shared_ptr<PolymorphicFutureBase> GetHandle() const noexcept;

	};

	export class Promise {
	private:
		UniquePromise m_impl;

	public:
		Promise(LogicalDevice const& logical_device);

		template <class Value>
		void SetValue(Value&& value) {
			m_impl->Visit(
				[&value](auto* derived) {
					derived->SetValue(std::forward<Value>(value));
				}
			);
		}

		Future GetFuture();

		PolymorphicPromiseBase* GetHandle() const noexcept;

	};

}