module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <type_traits>
#include <concepts>
#endif
module fyuu_rhi:future_impl;
#if defined(__cpp_lib_modules)
import std;              // Import the entire standard library if module support is available.
// In non‑module builds, the required headers were already included.
#endif
import plastic.sealed_polymorphism;
import :future;
import :types;
import :logical_device;
#if !defined(__APPLE__)
import :vulkan;
import :opengl;
#elif 
import :metal;
#endif // !defined(__APPLE__)
#if defined(_WIN32)
import :d3d12;
#endif // defined(_WIN32)
import :webgpu;

namespace fyuu_rhi {
	Future::Future(std::shared_ptr<PolymorphicFutureBase> const& impl) noexcept
		: m_impl(impl) {

	}

	void Future::Wait() {
		m_impl->Visit(
			[](auto* derived) {
				derived->Wait();
			}
		);
	}

    std::shared_ptr<PolymorphicFutureBase> Future::GetHandle() const noexcept {
        return m_impl;
    }

    Promise::Promise(LogicalDevice const& logical_device)
		: m_impl(
			[&logical_device]() {

				auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
					[](vulkan::VulkanLogicalDevice const* logical_device) -> UniquePromise {
						return plastic::utility::MakeUnique<vulkan::VulkanPromise>(*logical_device);
					},
					[](opengl::OpenGLLogicalDevice const* logical_device) -> UniquePromise {
						return plastic::utility::MakeUnique<opengl::OpenGLPromise>(*logical_device);
					},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
					[](d3d12::D3D12LogicalDevice const* logical_device) -> UniquePromise {
						return plastic::utility::MakeUnique<d3d12::D3D12Promise>(*logical_device);
					},
#endif // defined(_WIN32)
				#if defined(__APPLE__)
					[](metal::MetalLogicalDevice const* logical_device) -> UniquePromise {
						return plastic::utility::MakeUnique<metal::MetalPromise>(*logical_device);
					}
#endif // defined(__APPLE__)
					[](webgpu::WebGPULogicalDevice const* logical_device) -> UniquePromise {
						return plastic::utility::MakeUnique<webgpu::WebGPUPromise>(*logical_device);
					}
				};

				return logical_device.GetHandle()->Visit(visitor);

			}()) {

	}

	Future Promise::GetFuture() {
		return m_impl->Visit(
			[](auto* derived) -> std::shared_ptr<PolymorphicFutureBase> {
				return derived->GetFuture();
			}
		);
	}

    PolymorphicPromiseBase* Promise::GetHandle() const noexcept {
        return m_impl.get();
    }

}