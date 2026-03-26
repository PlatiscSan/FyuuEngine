/* view.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <memory>
#include <string>
#endif
module fyuu_rhi:view_impl;
#if defined(__cpp_lib_modules)
import std;
#endif
import :view;
import plastic.sealed_polymorphism;
import :types;
import :enums;
import :logical_device;
import :resource;
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

	View::View(LogicalDevice const& logical_device, Resource const& resource, BufferSize buffer_size, BufferSize offset)
		: m_impl(
			[&logical_device, &resource, buffer_size, offset]() {
				auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
					[buffer_size, offset](vulkan::VulkanLogicalDevice const* logical_device, vulkan::VulkanResource const* resource) -> std::shared_ptr<PolymorphicViewBase> {
						return std::make_shared<vulkan::VulkanView>(*logical_device, *resource, buffer_size, offset);
					},
					[buffer_size, offset](opengl::OpenGLLogicalDevice const* logical_device, opengl::OpenGLResource const* resource) -> std::shared_ptr<PolymorphicViewBase> {
						return std::make_shared<opengl::OpenGLView>(*logical_device, *resource, buffer_size, offset);
					},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
					[buffer_size, offset](d3d12::D3D12LogicalDevice const* logical_device, d3d12::D3D12Resource const* resource) -> std::shared_ptr<PolymorphicViewBase> {
						return std::make_shared<d3d12::D3D12View>(*logical_device, *resource, buffer_size, offset);
					},
#endif // defined(_WIN32)
#if defined(__APPLE__)
					[buffer_size, offset](metal::MetalLogicalDevice const* logical_device, metal::MetalResource const* resource) -> std::shared_ptr<PolymorphicViewBase> {
						return std::make_shared<metal::MetalView>(*logical_device, *resource, buffer_size, offset);
					},
#endif // defined(__APPLE__)
					[buffer_size, offset](webgpu::WebGPULogicalDevice const* logical_device, webgpu::WebGPUResource const* resource) -> std::shared_ptr<PolymorphicViewBase> {
						return std::make_shared<webgpu::WebGPUView>(*logical_device, *resource, buffer_size, offset);
					},
					[](auto const*, auto const*) -> std::shared_ptr<PolymorphicViewBase> {
						throw std::logic_error("various API");
					}
				};

				return plastic::utility::Visit(visitor, logical_device.GetHandle(), resource.GetHandle());

			}()) {
	}

	View::View(LogicalDevice const& logical_device, Resource const& resource, std::size_t base_mip_level, std::size_t level_count, std::size_t base_layer, std::size_t layer_count)
		: m_impl(
			[&logical_device, &resource, base_mip_level, level_count, base_layer, layer_count]() {
				auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
					[base_mip_level, level_count, base_layer, layer_count](vulkan::VulkanLogicalDevice const* logical_device, vulkan::VulkanResource const* resource) -> std::shared_ptr<PolymorphicViewBase> {
						return std::make_shared<vulkan::VulkanView>(*logical_device, *resource, base_mip_level, level_count, base_layer, layer_count);
					},
					[base_mip_level, level_count, base_layer, layer_count](opengl::OpenGLLogicalDevice const* logical_device, opengl::OpenGLResource const* resource) -> std::shared_ptr<PolymorphicViewBase> {
						return std::make_shared<opengl::OpenGLView>(*logical_device, *resource, base_mip_level, level_count, base_layer, layer_count);
					},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
					[base_mip_level, level_count, base_layer, layer_count](d3d12::D3D12LogicalDevice const* logical_device, d3d12::D3D12Resource const* resource) -> std::shared_ptr<PolymorphicViewBase> {
						return std::make_shared<d3d12::D3D12View>(*logical_device, *resource, base_mip_level, level_count, base_layer, layer_count);
					},
#endif // defined(_WIN32)
#if defined(__APPLE__)
					[base_mip_level, level_count, base_layer, layer_count](metal::MetalLogicalDevice const* logical_device, metal::MetalResource const* resource) -> std::shared_ptr<PolymorphicViewBase> {
						return std::make_shared<metal::MetalView>(*logical_device, *resource, base_mip_level, level_count, base_layer, layer_count);
					},
#endif // defined(__APPLE__)
					[base_mip_level, level_count, base_layer, layer_count](webgpu::WebGPULogicalDevice const* logical_device, webgpu::WebGPUResource const* resource) -> std::shared_ptr<PolymorphicViewBase> {
						return std::make_shared<webgpu::WebGPUView>(*logical_device, *resource, base_mip_level, level_count, base_layer, layer_count);
					},
					[](auto const*, auto const*) -> std::shared_ptr<PolymorphicViewBase> {
						throw std::logic_error("various API");
					}
				};

				return plastic::utility::Visit(visitor, logical_device.GetHandle(), resource.GetHandle());
			}()) {
	}

	View::operator PolymorphicViewBase*() const noexcept {
		return m_impl.get();
	}

	PolymorphicViewBase* View::GetHandle() const noexcept {
		return m_impl.get();
	}

}