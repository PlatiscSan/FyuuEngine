module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <optional>
#include <variant>
#include <span>
#include <format>
#endif
#include <webgpu/webgpu_cpp.h>

module fyuu_rhi:webgpu_resource_impl;

#if defined(__cpp_lib_modules)
import std;
#endif
import :webgpu_resource;
import :types;
import :webgpu_common;
import :webgpu_logical_device;

namespace {

	using namespace fyuu_rhi;

	wgpu::BufferUsage VideoMemoryTypeToMapUsage(VideoMemoryType type) noexcept {
		switch (type) {
		case VideoMemoryType::HostVisible:
			return wgpu::BufferUsage::MapWrite;
		case VideoMemoryType::DeviceReadback:
			return wgpu::BufferUsage::MapRead;
		default:
			return wgpu::BufferUsage::None;
		}
	}

	wgpu::BufferUsage DetermineBufferUsage(ResourceFlags flags, VideoMemoryType type) noexcept {
		wgpu::BufferUsage usage = wgpu::BufferUsage::None;

		if ((flags & ResourceFlags::UniformBuffer) != ResourceFlags::Unknown)
			usage |= wgpu::BufferUsage::Uniform;
		if ((flags & ResourceFlags::StorageBuffer) != ResourceFlags::Unknown)
			usage |= wgpu::BufferUsage::Storage;
		if ((flags & ResourceFlags::IndexBuffer) != ResourceFlags::Unknown)
			usage |= wgpu::BufferUsage::Index;
		if ((flags & ResourceFlags::VertexBuffer) != ResourceFlags::Unknown)
			usage |= wgpu::BufferUsage::Vertex;
		if ((flags & ResourceFlags::IndirectBuffer) != ResourceFlags::Unknown)
			usage |= wgpu::BufferUsage::Indirect;

		usage |= wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc;

		usage |= VideoMemoryTypeToMapUsage(type);

		return usage;
	}

	wgpu::TextureUsage DetermineTextureUsage(ResourceFlags flags) noexcept {
		wgpu::TextureUsage usage = wgpu::TextureUsage::None;

		if ((flags & ResourceFlags::SampledTexture) != ResourceFlags::Unknown)
			usage |= wgpu::TextureUsage::TextureBinding;
		if ((flags & ResourceFlags::StorageTexture) != ResourceFlags::Unknown)
			usage |= wgpu::TextureUsage::StorageBinding;
		if ((flags & ResourceFlags::RenderTarget) != ResourceFlags::Unknown)
			usage |= wgpu::TextureUsage::RenderAttachment;
		if ((flags & ResourceFlags::DepthStencil) != ResourceFlags::Unknown)
			usage |= wgpu::TextureUsage::RenderAttachment;

		usage |= wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc;

		return usage;
	}

	wgpu::TextureFormat DetermineTextureFormat(ResourceFlags flags) {

		auto format_bits = flags & ResourceFlags::AllFormats;

		using underlying = std::underlying_type_t<ResourceFlags>;
		if (std::popcount(static_cast<underlying>(format_bits)) != 1) {
			return wgpu::TextureFormat::Undefined;
		}
		if ((flags & ResourceFlags::BGRA8_Unorm) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::BGRA8Unorm;
		if ((flags & ResourceFlags::RGBA8_Uint) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::RGBA8Uint;
		if ((flags & ResourceFlags::RGBA8_Unorm) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::RGBA8Unorm;
		if ((flags & ResourceFlags::RGBA16_Unorm) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::RGBA16Unorm;
		if ((flags & ResourceFlags::RGBA16_Snorm) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::RGBA16Snorm;
		if ((flags & ResourceFlags::RGBA16_Sfloat) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::RGBA16Float;
		if ((flags & ResourceFlags::RGBA32_Sfloat) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::RGBA32Float;
		if ((flags & ResourceFlags::R8_Uint) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::R8Uint;
		if ((flags & ResourceFlags::R16_Float) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::R16Float;
		if ((flags & ResourceFlags::R32_Uint) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::R32Uint;
		if ((flags & ResourceFlags::R32_Float) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::R32Float;
		if ((flags & ResourceFlags::BC1_RGB_Unorm) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::BC1RGBAUnorm;
		if ((flags & ResourceFlags::BC1_RGB_SRGB) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::BC1RGBAUnormSrgb;
		if ((flags & ResourceFlags::BC1_RGBA_Unorm) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::BC1RGBAUnorm;
		if ((flags & ResourceFlags::BC1_RGBA_SRGB) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::BC1RGBAUnormSrgb;
		if ((flags & ResourceFlags::BC2_Unorm) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::BC2RGBAUnorm;
		if ((flags & ResourceFlags::BC2_SRGB) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::BC2RGBAUnormSrgb;
		if ((flags & ResourceFlags::BC3_Unorm) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::BC3RGBAUnorm;
		if ((flags & ResourceFlags::BC3_SRGB) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::BC3RGBAUnormSrgb;
		if ((flags & ResourceFlags::BC4_Unorm) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::BC4RUnorm;
		if ((flags & ResourceFlags::BC4_SRGB) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::BC4RUnorm;
		if ((flags & ResourceFlags::BC5_Unorm) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::BC5RGUnorm;
		if ((flags & ResourceFlags::BC5_SRGB) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::BC5RGUnorm;
		if ((flags & ResourceFlags::D32SFloat) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::Depth32Float;
		if ((flags & ResourceFlags::D24UnormS8Uint) != ResourceFlags::Unknown)
			return wgpu::TextureFormat::Depth24PlusStencil8;

		return wgpu::TextureFormat::Undefined;
	}

	std::pair<wgpu::TextureDimension, uint32_t> DetermineTextureDimensionAndLayers(ResourceFlags flags) noexcept {
		uint32_t arrayLayers = 1;
		if ((flags & ResourceFlags::Texture1D) != ResourceFlags::Unknown) {
			return {wgpu::TextureDimension::e1D, arrayLayers};
		}
		else if ((flags & ResourceFlags::Texture3D) != ResourceFlags::Unknown) {
			return {wgpu::TextureDimension::e3D, arrayLayers};
		}
		else if ((flags & ResourceFlags::TextureCube) != ResourceFlags::Unknown) {
			arrayLayers = 6;
			return {wgpu::TextureDimension::e2D, arrayLayers};
		}
		else {
			return {wgpu::TextureDimension::e2D, arrayLayers};
		}
	}

} // anonymous namespace

namespace fyuu_rhi::webgpu {


	WebGPUResource::WebGPUResource(
		WebGPULogicalDevice const& device, 
		BufferSize buffer_size,
		VideoMemoryType type, 
		ResourceFlags flags,
		std::string_view debug_name
	) : PolymorphicResourceBase(this),
		WebGPUCommon(this),
		m_handle(
			[&device, buffer_size, type, flags, debug_name]() {
				bool is_buffer = (flags & ResourceFlags::Buffer) != ResourceFlags::Unknown;
				bool is_texture = (flags & ResourceFlags::Texture) != ResourceFlags::Unknown;

				if (is_texture) {
					throw std::runtime_error("trying to create buffer but texture flag is set");
				}
				if (!is_buffer) {
					throw std::runtime_error("trying to create buffer but no buffer flag is set");
				}

				wgpu::BufferDescriptor desc;
				desc.size = buffer_size;
				desc.usage = DetermineBufferUsage(flags, type);
				desc.mappedAtCreation = false;

				wgpu::Buffer buffer = device.CreateBuffer(desc, debug_name);
				if (!buffer) {
					throw std::runtime_error("Failed to create WebGPU buffer");
				}
				return buffer;
			}()) {
			
	}

	WebGPUResource::WebGPUResource(
		WebGPULogicalDevice const& device, 
		TextureSize const& texture_size,
		VideoMemoryType type, 
		ResourceFlags flags,
		std::string_view debug_name
	) : PolymorphicResourceBase(this),
		WebGPUCommon(this),
		m_handle(
			[&device, &texture_size, flags, debug_name]() {
				bool is_buffer = (flags & ResourceFlags::Buffer) != ResourceFlags::Unknown;
				bool is_texture = (flags & ResourceFlags::Texture) != ResourceFlags::Unknown;

				if (is_buffer) {
					throw std::runtime_error("trying to create texture but buffer flag is set");
				}
				if (!is_texture) {
					throw std::runtime_error("trying to create texture but no texture flag is set");
				}

				wgpu::TextureFormat format = DetermineTextureFormat(flags);
				if (format == wgpu::TextureFormat::Undefined) {
					throw std::runtime_error("Invalid or ambiguous texture format in flags");
				}

				auto [dimension, array_layers] = DetermineTextureDimensionAndLayers(flags);

				wgpu::TextureDescriptor desc;
				desc.dimension = dimension;
				desc.size = {
					static_cast<uint32_t>(texture_size.width),
					static_cast<uint32_t>(texture_size.height),
					static_cast<uint32_t>(texture_size.depth)
				};
				if (dimension == wgpu::TextureDimension::e3D) {
				} else {
					desc.size.depthOrArrayLayers = array_layers;
				}
				desc.format = format;
				desc.usage = DetermineTextureUsage(flags);
				desc.mipLevelCount = 1;
				desc.sampleCount = 1;

				wgpu::Texture texture = device.CreateTexture(desc, debug_name);
				if (!texture) {
					throw std::runtime_error("Failed to create WebGPU texture");
				}
				return texture;
			}()) {

	}

	WebGPUResource::WebGPUResource(
		wgpu::Texture back_buffer, 
		TextureSize const& texture_size,
		wgpu::TextureFormat format
	) : PolymorphicResourceBase(this),
		WebGPUCommon(this),
		m_handle(BackBuffer{back_buffer, texture_size, format}) {

	}

} // namespace fyuu_rhi::webgpu