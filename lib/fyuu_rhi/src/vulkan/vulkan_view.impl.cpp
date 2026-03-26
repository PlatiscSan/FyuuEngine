/* vulkan_view.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <string>
#include <variant>
#include <format>
#include <concepts>
#endif
module fyuu_rhi:vulkan_view_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import :vulkan_view;
import vulkan;
import plastic.registrable;
import plastic.resource;
import :vulkan_types;
import :enums;
import :vulkan_logical_device;
import :vulkan_resource;
import :vulkan_common;

namespace fyuu_rhi::vulkan {

	VulkanView::VulkanView(VulkanLogicalDevice const& logical_device, VulkanResource const& back_buffer, std::size_t index)
    	: PolymorphicViewBase(this),
		VulkanCommon(this),
		m_handle(
			[&logical_device, &back_buffer, index]() {

				vk::ImageViewCreateInfo info(
					vk::ImageViewCreateFlags{},
					back_buffer.GetNative().texture,
					vk::ImageViewType::e2D,
					back_buffer.GetFormat(),
					{
						vk::ComponentSwizzle::eIdentity,
						vk::ComponentSwizzle::eIdentity,
						vk::ComponentSwizzle::eIdentity,
						vk::ComponentSwizzle::eIdentity
					},
					{
						vk::ImageAspectFlagBits::eColor,
						0u,
						1u,
						0u,
						1u
					},
					nullptr
				);

				TextureView tv{
					{
						vk::ImageViewCreateFlags{},
						back_buffer.GetNative().texture,
						vk::ImageViewType::e2D,
						back_buffer.GetFormat(),
						{
							vk::ComponentSwizzle::eIdentity,
							vk::ComponentSwizzle::eIdentity,
							vk::ComponentSwizzle::eIdentity,
							vk::ComponentSwizzle::eIdentity
						},
						{
							vk::ImageAspectFlagBits::eColor,
							0u,
							1u,
							0u,
							1u
						},
						nullptr
					},
					{}
				};

				std::string debug_name = std::format("Swap chain buffer {}", index);
				tv.impl = logical_device.CreateTextureView(info, debug_name);

				return tv;

			}()) {
    }

	VulkanView::VulkanView(VulkanLogicalDevice const& logical_device, VulkanResource const& resource, BufferSize buffer_size, BufferSize offset)
		: PolymorphicViewBase(this),
		VulkanCommon(this),
		m_handle(
			[&logical_device, &resource, buffer_size, offset]() -> Handle {
				vk::Format format = resource.GetFormat();
				if (format == vk::Format::eUndefined) {
					throw std::invalid_argument("Buffer format is undefined");
				}
				BufferView bv{
					{
						vk::BufferViewCreateFlags{},				// flags
						resource.GetNative().buffer,				// buffer
						format,										// format
						static_cast<vk::DeviceSize>(offset),		// offset
						static_cast<vk::DeviceSize>(buffer_size),	// range (use whole buffer)
						nullptr
					},
					{}
				};
			
				bv.impl = logical_device.CreateBufferView(bv.info);

				return bv;
			}()) {
	}

	VulkanView::VulkanView(VulkanLogicalDevice const& logical_device, VulkanResource const& resource, std::size_t base_mip_level, std::size_t level_count, std::size_t base_layer, std::size_t layer_count)
		: PolymorphicViewBase(this),
		VulkanCommon(this),
		m_handle(
			[&logical_device, &resource, base_mip_level, level_count, base_layer, layer_count]() -> Handle {

				auto tex_desc = resource.GetTextureDescription();
				if (!tex_desc) {
					throw std::invalid_argument("Resource is not a texture");
				}

				if (base_mip_level >= tex_desc->mipLevels) {
					throw std::out_of_range("base_mip_level out of range");
				}
				if (level_count == 0 || base_mip_level + level_count > tex_desc->mipLevels) {
					throw std::out_of_range("level_count out of range");
				}
				if (base_layer >= tex_desc->arrayLayers) {
					throw std::out_of_range("base_layer out of range");
				}
				if (layer_count == 0 || base_layer + layer_count > tex_desc->arrayLayers) {
					throw std::out_of_range("layer_count out of range");
				}

				vk::Format format = tex_desc->format;

				vk::ImageViewType view_type;
				if (layer_count >= 6 && (layer_count % 6 == 0) && (base_layer % 6 == 0)) {
					view_type = (layer_count > 6) ? vk::ImageViewType::eCubeArray : vk::ImageViewType::eCube;
				} 
				else {
					switch (tex_desc->imageType) {
					case vk::ImageType::e1D:
						view_type = (layer_count > 1) ? vk::ImageViewType::e1DArray : vk::ImageViewType::e1D;
						break;
					case vk::ImageType::e2D:
						view_type = (layer_count > 1) ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D;
						break;
					case vk::ImageType::e3D:
						view_type = vk::ImageViewType::e3D;
						break;
					default:
						view_type = vk::ImageViewType::e2D;
						break;
					}
				}
			
				vk::ImageAspectFlags aspect;
				switch (tex_desc->format) {
				case vk::Format::eD16Unorm:
				case vk::Format::eX8D24UnormPack32:
				case vk::Format::eD32Sfloat:
					aspect = vk::ImageAspectFlagBits::eDepth;
					break;
				case vk::Format::eS8Uint:
					aspect = vk::ImageAspectFlagBits::eStencil;
					break;
				case vk::Format::eD16UnormS8Uint:
				case vk::Format::eD24UnormS8Uint:
				case vk::Format::eD32SfloatS8Uint:
					aspect = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
					break;
				default:
					aspect = vk::ImageAspectFlagBits::eColor;
					break;
				}

				
				TextureView tv{
					{
						vk::ImageViewCreateFlags{},
						resource.GetNative().texture,
						view_type,
						tex_desc->format,
						{
							vk::ComponentSwizzle::eIdentity,
							vk::ComponentSwizzle::eIdentity,
							vk::ComponentSwizzle::eIdentity,
							vk::ComponentSwizzle::eIdentity
						},
						{
							aspect,
							static_cast<std::uint32_t>(base_mip_level),
							static_cast<std::uint32_t>(level_count),
							static_cast<std::uint32_t>(base_layer),
							static_cast<std::uint32_t>(layer_count)
						},
						nullptr
					},
					{}
				};

				tv.impl = logical_device.CreateTextureView(tv.info);

				return tv;

			}()) {
	}

	VulkanView::Native VulkanView::GetNative() const noexcept {
		return std::visit(
			[](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				Native native;
				if constexpr (std::same_as<Handle, SharedBufferView>) {
					native.buffer = *handle->impl;
				}
				else if constexpr (std::same_as<Handle, SharedTextureView>) {
					native.texture = *handle->impl;
				}
				else {
					native.texture = nullptr;
				}
				return native;
			},
			m_handle
		);
	}
	vk::ImageViewCreateInfo const* VulkanView::GetTextureDescription() const noexcept {
		return std::visit(
			[](auto&& handle) -> vk::ImageViewCreateInfo const* {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, SharedTextureView>) {
					return &handle->info;
				} 
				else {
					return nullptr;
				}
			},
			m_handle
		);
	}

	vk::BufferViewCreateInfo const* VulkanView::GetBufferDescription() const noexcept {
		return std::visit(
			[](auto&& handle) -> vk::BufferViewCreateInfo const* {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, SharedBufferView>) {
					return &handle->info;
				} 
				else {
					return nullptr;
				}
			},
			m_handle
		);
	}
}

#endif // !defined(__APPLE__)