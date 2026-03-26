module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <utility>
#include <memory>
#include <tuple>
#include <optional>
#include <string_view>
#include <format>
#include <print>
#endif // !defined(__cpp_lib_modules)
module fyuu_rhi:swap_chain_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :swap_chain;
import plastic.sealed_polymorphism;
import :physical_device;
import :logical_device;
import :command_queue;
import :future;
import :types;
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
	
	SwapChain::SwapChain(
		PhysicalDevice const& physical_device,
		LogicalDevice const& logical_device,
		CommandQueue const& present_queue,
		Surface const& surface,
		SwapChainOption const& swap_chain_option
	) : m_impl(
		[&]() -> UniqueSwapChain {
			auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
				[&swap_chain_option](vulkan::VulkanPhysicalDevice const* physical_device, vulkan::VulkanLogicalDevice const* logical_device, vulkan::VulkanCommandQueue const* present_queue, vulkan::VulkanSurface const* surface) -> UniqueSwapChain {
					return plastic::utility::MakeUnique<vulkan::VulkanSwapChain>(
						*physical_device,
						*logical_device,
						*present_queue,
						*surface,
						swap_chain_option
					);
				},
				[&swap_chain_option](opengl::OpenGLPhysicalDevice const* physical_device, opengl::OpenGLLogicalDevice const* logical_device, opengl::OpenGLCommandQueue const* present_queue, opengl::OpenGLSurface const* surface) -> UniqueSwapChain {
					return plastic::utility::MakeUnique<opengl::OpenGLSwapChain>(
						*physical_device,
						*logical_device,
						*present_queue,
						*surface,
						swap_chain_option
					);
				},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
				[&swap_chain_option](d3d12::D3D12PhysicalDevice const* physical_device, d3d12::D3D12LogicalDevice const* logical_device, d3d12::D3D12CommandQueue const* present_queue, d3d12::D3D12Surface const* surface) -> UniqueSwapChain {
					return plastic::utility::MakeUnique<d3d12::D3D12SwapChain>(
						*physical_device,
						*logical_device,
						*present_queue,
						*surface,
						swap_chain_option
					);
				},
#endif // defined(_WIN32)
#if defined(__APPLE__)
				[&swap_chain_option](metal::MetalPhysicalDevice const* physical_device, metal::MetalLogicalDevice const* logical_device, metal::MetalCommandQueue const* present_queue, metal::MetalSurface const* surface) -> UniqueSwapChain {
					return plastic::utility::MakeUnique<metal::MetalSwapChain>(
						*physical_device,
						*logical_device,
						*present_queue,
						*surface,
						swap_chain_option
					);
				}
#endif // defined(__APPLE__)
				[&swap_chain_option](webgpu::WebGPUPhysicalDevice const* physical_device, webgpu::WebGPULogicalDevice const* logical_device, webgpu::WebGPUCommandQueue const* present_queue, webgpu::WebGPUSurface const* surface) -> UniqueSwapChain {
					return plastic::utility::MakeUnique<webgpu::WebGPUSwapChain>(
						*physical_device,
						*logical_device,
						*present_queue,
						*surface,
						swap_chain_option
					);
				},
				[](auto const*, auto const*, auto const*, auto const*) -> UniqueSwapChain {
					throw std::logic_error("various API");
				}
			};

			return plastic::utility::Visit(
				visitor,
				physical_device.GetHandle(),
				logical_device.GetHandle(),
				present_queue.GetHandle(),
				surface.GetHandle()
			);

		}()) {

	}

	void SwapChain::Resize(std::optional<std::size_t> const& back_buffer_size) {
		m_impl->Visit(
			[&back_buffer_size](auto* derived) {
				derived->Resize(back_buffer_size);
			}
		);
	}

	std::tuple<PolymorphicResourceBase*, PolymorphicViewBase*, Future> SwapChain::GetNextFrame() {
		return m_impl->Visit(
			[](auto* derived) -> std::tuple<PolymorphicResourceBase*, PolymorphicViewBase*, Future> {
				return derived->GetNextFrame();
			}
		);
	}
	
	void SwapChain::Present(Future const& future) {

		auto future_handle = future.GetHandle();
		if (!future_handle) {
			m_impl->Visit(
				[](auto* swap_chain) {
					swap_chain->Present(nullptr);
				}
			);
			return;
		}

		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[](vulkan::VulkanSwapChain* swap_chain, std::shared_ptr<vulkan::VulkanFuture> const& future) {
				swap_chain->Present(future);
			},
			[](opengl::OpenGLSwapChain* swap_chain, std::shared_ptr<opengl::OpenGLFuture> const& future) {
				swap_chain->Present(future);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[](d3d12::D3D12SwapChain* swap_chain, std::shared_ptr<d3d12::D3D12Future> const& future) {
				swap_chain->Present(future);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[](metal::MetalSwapChain* swap_chain, std::shared_ptr<metal::MetalFuture> const& future) {
				swap_chain->Present(future);
			},
#endif // defined(__APPLE__)
			[](webgpu::WebGPUSwapChain* swap_chain, std::shared_ptr<webgpu::WebGPUFuture> const& future) {
				swap_chain->Present(future);
			},
			[](auto&&, auto&&) {
				throw std::logic_error("various API");
			}
		};

		plastic::utility::Visit(visitor, m_impl.get(), future.GetHandle());

	}

	void SwapChain::SetVSync(bool mode) {
		m_impl->Visit(
			[mode](auto* derived) {
				return derived->SetVSync(mode);
			}
		);
	}


}