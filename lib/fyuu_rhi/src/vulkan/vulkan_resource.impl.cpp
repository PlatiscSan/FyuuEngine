module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <optional>
#include <variant>
#include <span>
#include <format>
#include <concepts>
#endif // !defined(__cpp_lib_modules)
#include <vma/vk_mem_alloc.h>
module fyuu_rhi:vulkan_resource_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :vulkan_resource;
import vulkan;
import plastic.resource;
import plastic.registrable;
import :types;
import :vulkan_common;
import :vulkan_logical_device;

namespace {

	using namespace fyuu_rhi;
	using namespace fyuu_rhi::vulkan;

	vk::BufferUsageFlags DetermineBufferUsage(ResourceFlags flags) noexcept {

		vk::BufferUsageFlags usage{};
		if ((flags & ResourceFlags::UniformTexelBuffer) != ResourceFlags::Unknown)
			usage |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
		if ((flags & ResourceFlags::StorageTexelBuffer) != ResourceFlags::Unknown)
			usage |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
		if ((flags & ResourceFlags::UniformBuffer) != ResourceFlags::Unknown)
			usage |= vk::BufferUsageFlagBits::eUniformBuffer;
		if ((flags & ResourceFlags::StorageBuffer) != ResourceFlags::Unknown)
			usage |= vk::BufferUsageFlagBits::eStorageBuffer;
		if ((flags & ResourceFlags::IndexBuffer) != ResourceFlags::Unknown)
			usage |= vk::BufferUsageFlagBits::eIndexBuffer;
		if ((flags & ResourceFlags::VertexBuffer) != ResourceFlags::Unknown)
			usage |= vk::BufferUsageFlagBits::eVertexBuffer;
		if ((flags & ResourceFlags::IndirectBuffer) != ResourceFlags::Unknown)
			usage |= vk::BufferUsageFlagBits::eIndirectBuffer;

		usage |= vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc;
		return usage;
	}

	vk::ImageUsageFlags DetermineTextureUsage(ResourceFlags flags) noexcept {

		vk::ImageUsageFlags usage{};
		if ((flags & ResourceFlags::SampledTexture) != ResourceFlags::Unknown)
			usage |= vk::ImageUsageFlagBits::eSampled;
		if ((flags & ResourceFlags::StorageTexture) != ResourceFlags::Unknown)
			usage |= vk::ImageUsageFlagBits::eStorage;
		if ((flags & ResourceFlags::RenderTarget) != ResourceFlags::Unknown)
			usage |= vk::ImageUsageFlagBits::eColorAttachment;
		if ((flags & ResourceFlags::DepthStencil) != ResourceFlags::Unknown)
			usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
		if ((flags & ResourceFlags::Input) != ResourceFlags::Unknown)
			usage |= vk::ImageUsageFlagBits::eInputAttachment;

		usage |= vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
		return usage;

	}

	std::tuple<vk::ImageType, vk::ImageCreateFlags, std::uint32_t> DetermineTextureTypeAndFlags(ResourceFlags flags) noexcept {
		vk::ImageType type = vk::ImageType::e2D;
		vk::ImageCreateFlags create_flags = {};
		std::uint32_t array_layers = 1u;

		if ((flags & ResourceFlags::Texture1D) != ResourceFlags::Unknown) {
			type = vk::ImageType::e1D;
		}
		else if ((flags & ResourceFlags::Texture2D) != ResourceFlags::Unknown) {
			type = vk::ImageType::e2D;
		}
		else if ((flags & ResourceFlags::Texture3D) != ResourceFlags::Unknown) {
			type = vk::ImageType::e3D;
		}
		else if ((flags & ResourceFlags::TextureCube) != ResourceFlags::Unknown) {
			type = vk::ImageType::e2D;
			create_flags |= vk::ImageCreateFlagBits::eCubeCompatible;
			array_layers = 6;
		}

		return { type, create_flags, array_layers };
	}

	VmaMemoryUsage VideoMemoryTypeToVmaMemoryUsage(VideoMemoryType type) noexcept {
		switch (type) {
		case VideoMemoryType::DeviceLocal:   return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		case VideoMemoryType::HostVisible:   return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		case VideoMemoryType::DeviceReadback:return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		default:                             return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		}
	}

	VmaAllocationCreateFlags VideoMemoryTypeToVmaCreateFlags(VideoMemoryType type) noexcept {
		switch (type) {
		case VideoMemoryType::DeviceLocal:   return 0;
		case VideoMemoryType::HostVisible:   return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		case VideoMemoryType::DeviceReadback:return VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		default:                             return 0;
		}
	}
	
	VulkanResourceState DetermineInitialState(VideoMemoryType type) noexcept {
		switch (type) {
		case VideoMemoryType::DeviceLocal:
			return {
				.stage = vk::PipelineStageFlagBits2::eTopOfPipe,
				.access = vk::AccessFlagBits2::eNone,
				.layout = vk::ImageLayout::eUndefined
			};
		case VideoMemoryType::HostVisible:
			return {
				.stage = vk::PipelineStageFlagBits2::eHost,
				.access = vk::AccessFlagBits2::eHostWrite,
				.layout = vk::ImageLayout::ePreinitialized
			};
		case VideoMemoryType::DeviceReadback:
			return {
				.stage = vk::PipelineStageFlagBits2::eTopOfPipe,
				.access = vk::AccessFlagBits2::eNone,
				.layout = vk::ImageLayout::eUndefined
			};
		default:
			return {
				.stage = vk::PipelineStageFlagBits2::eTopOfPipe,
				.access = vk::AccessFlagBits2::eNone,
				.layout = vk::ImageLayout::eUndefined
			};
		}
	}

	vk::ImageTiling DetermineTextureTiling(VideoMemoryType type) noexcept {
		return (type == VideoMemoryType::DeviceLocal)
			? vk::ImageTiling::eOptimal
			: vk::ImageTiling::eLinear;
	}

}

namespace fyuu_rhi::vulkan {

	VulkanResource::BufferHandleGC::BufferHandleGC(VulkanLogicalDevice const& logical_device) noexcept
		: m_logical_device_id(logical_device.GetID()) {
	}

	void VulkanResource::BufferHandleGC::operator()(BufferHandle& handle) noexcept {

		if (!m_logical_device_id) {
			return;
		}

		VulkanLogicalDevice* logical_device = plastic::utility::QueryObject<VulkanLogicalDevice>(*m_logical_device_id);

		if (!logical_device) {
			return;
		}

		vmaDestroyBuffer(logical_device->GetVideoMemoryAllocator(), handle.impl, handle.allocation);

	}

	VulkanResource::TextureHandleGC::TextureHandleGC(VulkanLogicalDevice const& logical_device) noexcept
		: m_logical_device_id(logical_device.GetID()) {
	}

	void VulkanResource::TextureHandleGC::operator()(TextureHandle& handle) noexcept {

		if (!m_logical_device_id) {
			return;
		}

		VulkanLogicalDevice* logical_device = plastic::utility::QueryObject<VulkanLogicalDevice>(*m_logical_device_id);

		if (!logical_device) {
			return;
		}

		vmaDestroyImage(logical_device->GetVideoMemoryAllocator(), handle.impl, handle.allocation);

	}

	VulkanResource::VulkanResource(VulkanLogicalDevice const& logical_device, BufferSize buffer_size, VideoMemoryType type, ResourceFlags flags)
		: PolymorphicResourceBase(this),
		VulkanCommon(this),
		m_state(vk::PipelineStageFlagBits2::eTopOfPipe, vk::AccessFlagBits2::eNone, vk::ImageLayout::eUndefined),
		m_handle(
			[&logical_device, buffer_size, type, flags]() {

				bool is_buffer = (flags & ResourceFlags::Buffer) != ResourceFlags::Unknown;
				bool is_texture = (flags & ResourceFlags::Texture) != ResourceFlags::Unknown;

				if (is_texture) {
					throw std::runtime_error("trying to create buffer but texture flag is set");
				}

				if (!is_buffer) {
					throw std::runtime_error("trying to create buffer but no buffer flag is set");
				}

				auto allocator = logical_device.GetVideoMemoryAllocator();

				BufferHandle buffer_handle{
					{
						{}, 
						buffer_size, 
						DetermineBufferUsage(flags), 
						vk::SharingMode::eExclusive, 
						0u, 
						nullptr,
						nullptr
					},
					nullptr,
					nullptr,
					{},
					DetermineFormat(flags)
				};

				VmaAllocationCreateInfo allocation_create_info{
					VideoMemoryTypeToVmaCreateFlags(type),
					VideoMemoryTypeToVmaMemoryUsage(type),
					0, 0, 0, nullptr, nullptr, 0.0f
				};

				auto result = static_cast<vk::Result>(
					vmaCreateBuffer(
						allocator,
						reinterpret_cast<VkBufferCreateInfo const*>(&buffer_handle.buffer_info),
						&allocation_create_info,
						reinterpret_cast<VkBuffer*>(&buffer_handle.impl),
						&buffer_handle.allocation,
						&buffer_handle.alloc_info
					));

				if (result != vk::Result::eSuccess) {
					throw std::runtime_error(std::format("vmaCreateBuffer failed, Vulkan result: {}", static_cast<std::size_t>(result)));
				}

				return UniqueBufferHandle(buffer_handle, logical_device);

			}()) {
	}

	VulkanResource::VulkanResource(VulkanLogicalDevice const& logical_device, TextureSize const& texture_size, VideoMemoryType type, ResourceFlags flags)
		: PolymorphicResourceBase(this),
		VulkanCommon(this),
		m_state(DetermineInitialState(type)),
		m_handle(
			[this, &logical_device, &texture_size, type, flags]() {

				bool is_buffer = (flags & ResourceFlags::Buffer) != ResourceFlags::Unknown;
				bool is_texture = (flags & ResourceFlags::Texture) != ResourceFlags::Unknown;

				if (is_buffer) {
					throw std::runtime_error("trying to create texture but buffer flag is set");
				}
				if (!is_texture) {
					throw std::runtime_error("trying to create texture but no texture flag is set");
				}

				auto [texture_type, create_flags, array_layers] = DetermineTextureTypeAndFlags(flags);
				vk::ImageTiling tiling = DetermineTextureTiling(type);

				TextureHandle texture_handle{
					{
						create_flags,
						texture_type,
						DetermineFormat(flags),
						{ static_cast<std::uint32_t>(texture_size.width), static_cast<std::uint32_t>(texture_size.height), static_cast<std::uint32_t>(texture_size.depth) },
						1u,
						array_layers,
						vk::SampleCountFlagBits::e1,
						tiling,
						DetermineTextureUsage(flags),
						vk::SharingMode::eExclusive,
						0u,
						nullptr,
						m_state.layout,
						nullptr
					}, 
					nullptr, 
					nullptr, 
					{}
				};

				VmaAllocationCreateInfo allocation_create_info{
					VideoMemoryTypeToVmaCreateFlags(type),
					VideoMemoryTypeToVmaMemoryUsage(type),
					0, 0, 0, nullptr, nullptr, 0.0f
				};

				auto allocator = logical_device.GetVideoMemoryAllocator();

				auto result = static_cast<vk::Result>(
					vmaCreateImage(
						allocator,
						reinterpret_cast<VkImageCreateInfo const*>(&texture_handle.texture_info),
						&allocation_create_info,
						reinterpret_cast<VkImage*>(&texture_handle.impl),
						&texture_handle.allocation,
						&texture_handle.alloc_info
					));

				if (result != vk::Result::eSuccess) {
					throw std::runtime_error(std::format("vmaCreateImage failed, Vulkan result: {}", static_cast<std::size_t>(result)));
				}

				return UniqueTextureHandle(texture_handle, logical_device);

			}()) {
	}

	VulkanResource::VulkanResource(vk::Image back_buffer, vk::Format back_buffer_format, std::size_t width, std::size_t height)
		:PolymorphicResourceBase(this),
		VulkanCommon(this),
		m_state(vk::PipelineStageFlagBits2::eTopOfPipe, vk::AccessFlagBits2::eNone, vk::ImageLayout::eUndefined),
		m_handle(
			BackBuffer{
				back_buffer,
				{
					width,
					height,
					1u
				},
				back_buffer_format
			}
		) {

	}

	VulkanResource::Native VulkanResource::GetNative() const noexcept {
		return std::visit(
			[](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				Native native;
				if constexpr (std::same_as<Handle, UniqueBufferHandle>) {
					native.buffer = handle->impl;
				}
				else if constexpr (std::same_as<Handle, UniqueTextureHandle>) {
					native.texture = handle->impl;
				} 
				else if constexpr (std::same_as<Handle, BackBuffer>) {
					native.texture = handle.impl;
				} 
				else {
					native.texture = nullptr;
				}
				return native;
			},
			m_handle
		);
	}

	vk::Format VulkanResource::GetFormat() const noexcept {
		return std::visit(
			[](auto&& handle) -> vk::Format {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, UniqueTextureHandle>) {
					return handle->texture_info.format;
				} 
				else if constexpr (std::same_as<Handle, BackBuffer>) {
					return handle.format;
				} 
				else if constexpr (std::same_as<Handle, UniqueBufferHandle>) {
					return handle->format;
				}
				else {
					return vk::Format::eUndefined;
				}
			},
			m_handle
		);
	}

	vk::ImageCreateInfo const* VulkanResource::GetTextureDescription() const noexcept {
		return std::visit(
			[](auto&& handle) -> vk::ImageCreateInfo const* {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, UniqueTextureHandle>) {
					return &handle->texture_info;
				} 
				else {
					return nullptr;
				}
			},
			m_handle
		);
	}

	vk::BufferCreateInfo const* VulkanResource::GetBufferDescription() const noexcept {
		return std::visit(
			[](auto&& handle) -> vk::BufferCreateInfo const* {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, UniqueBufferHandle>) {
					return &handle->buffer_info;
				} 
				else {
					return nullptr;
				}
			},
			m_handle
		);
	}

	VulkanResourceState const& VulkanResource::GetState() const noexcept {
		return m_state;
	}

	void VulkanResource::SetState(VulkanResourceState const& state) noexcept {
		m_state = state;
	}

	TextureSize VulkanResource::GetTextureSize() const noexcept {
		return std::visit(
			[](auto&& handle) -> TextureSize {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, UniqueTextureHandle>) {
					return {
						handle->texture_info.extent.width,
						handle->texture_info.extent.height,
						handle->texture_info.extent.depth
					};
				} 
				else if constexpr (std::same_as<Handle, BackBuffer>) {
					return handle.size;
				}
				else {
					return {};
				}
			},
			m_handle
		);
	}

	vk::ImageAspectFlags VulkanResource::GetAspectFlags() const noexcept {
		switch (GetFormat()) {
		case vk::Format::eD16Unorm:
		case vk::Format::eX8D24UnormPack32:
		case vk::Format::eD32Sfloat:
			return vk::ImageAspectFlagBits::eDepth;
		case vk::Format::eS8Uint:
			return vk::ImageAspectFlagBits::eStencil;
		case vk::Format::eD16UnormS8Uint:
		case vk::Format::eD24UnormS8Uint:
		case vk::Format::eD32SfloatS8Uint:
			return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		default:
			return vk::ImageAspectFlagBits::eColor;
		}
	}

	vk::ImageSubresourceRange VulkanResource::WholeResourceRange() const noexcept {
		return std::visit(
			[this](auto&& handle) -> vk::ImageSubresourceRange {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, UniqueTextureHandle>) {
					return vk::ImageSubresourceRange(
						GetAspectFlags(),
						0,
						handle->texture_info.mipLevels,
						0,
						handle->texture_info.arrayLayers
					);
				} 
				else if constexpr (std::same_as<Handle, BackBuffer>) {
					return vk::ImageSubresourceRange(
						GetAspectFlags(),
						0,
						1,  // mipLevels
						0,
						1   // arrayLayers
					);
				} 
				else {
					return vk::ImageSubresourceRange();
				}
			},
			m_handle
		);
	}
}

#endif // !defined(__APPLE__)