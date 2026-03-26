/* command_queue.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string_view>
#include <print>
#endif // !defined(__cpp_lib_modules)
module fyuu_rhi:command_queue_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import plastic.sealed_polymorphism;
import :command_queue;
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
	
	CommandQueue::CommandQueue(LogicalDevice& logical_device, CommandObjectType type, QueuePriority priority)
		: m_impl(
			[&logical_device, type, priority]() { 
				auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
					[type, priority](vulkan::VulkanLogicalDevice* logical_device) -> UniqueCommandQueue {
						return plastic::utility::MakeUnique<vulkan::VulkanCommandQueue>(*logical_device, type, priority);
					},
					[type, priority](opengl::OpenGLLogicalDevice const* logical_device) -> UniqueCommandQueue {
						return plastic::utility::MakeUnique<opengl::OpenGLCommandQueue>(*logical_device, type, priority);
					},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
					[type, priority](d3d12::D3D12LogicalDevice const* logical_device) -> UniqueCommandQueue {
						return plastic::utility::MakeUnique<d3d12::D3D12CommandQueue>(*logical_device, type, priority);
					},
#endif // defined(_WIN32)
#if defined(__APPLE__)
					[type, priority](metal::MetalLogicalDevice const* logical_device) -> UniqueCommandQueue {
						return plastic::utility::MakeUnique<metal::MetalCommandQueue>(*logical_device, type, priority);
					}
#endif // defined(__APPLE__)
					[type, priority](webgpu::WebGPULogicalDevice const* logical_device) -> UniqueCommandQueue {
						return plastic::utility::MakeUnique<webgpu::WebGPUCommandQueue>(*logical_device, type, priority);
					}
				};

				return logical_device.GetHandle()->Visit(visitor);

			}()) {

	}

	void CommandQueue::WaitUntilCompleted() {
		m_impl->Visit(
			[](auto* derived) {
				derived->WaitUntilCompleted();
			}
		);
	}

    PolymorphicCommandQueueBase* CommandQueue::GetHandle() const noexcept {
        return m_impl.get();
    }

    //Future CommandQueue::ExecuteCommand(CommandBuffer& command_buffer, Promise& promise, Future const& future) {
	//	return {};
	//}

    API CommandQueue::GetAPI() const noexcept {
        return m_impl->Visit(
			[](auto const* derived) {
				return derived->GetAPI();
			}
		);
    }

	std::string_view CommandQueue::GetAPIString() const noexcept {
        return m_impl->Visit(
			[](auto const* derived) {
				return derived->GetAPIString();
			}
		);
	}	

} // namespace fyuu_rhi

