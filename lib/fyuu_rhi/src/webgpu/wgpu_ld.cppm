module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#endif // !defined(__cpp_lib_modules)
#include <webgpu/webgpu_cpp.h>
module fyuu_rhi:webgpu_logical_device;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :webgpu_traits;
import :command_types;
import :resource_types;

namespace {

	using namespace fyuu_rhi;

	wgpu::BufferUsage ExtractBufferUsageFlags(ResourceFlags const& flags) {

		wgpu::BufferUsage wgpu_flags{};
	
		if (flags.Test(ResourceFlagBits::CopySRC)) {
			wgpu_flags |= wgpu::BufferUsage::CopySrc;
		}
		if (flags.Test(ResourceFlagBits::CopyDST)) {
			wgpu_flags |= wgpu::BufferUsage::CopyDst;
		}
		if (flags.Test(ResourceFlagBits::UniformTexelBuffer)) {
			wgpu_flags |= wgpu::BufferUsage::TexelBuffer;
		}
		if (flags.Test(ResourceFlagBits::StorageTexelBuffer)) {
			wgpu_flags |= wgpu::BufferUsage::TexelBuffer;
		}
		if (flags.Test(ResourceFlagBits::UniformBuffer)) {
			wgpu_flags |= wgpu::BufferUsage::Uniform;
		}
		if (flags.Test(ResourceFlagBits::StorageBuffer)) {
			wgpu_flags |= wgpu::BufferUsage::Storage;
		}
		if (flags.Test(ResourceFlagBits::IndexBuffer)) {
			wgpu_flags |= wgpu::BufferUsage::Index;
		}
		if (flags.Test(ResourceFlagBits::VertexBuffer)) {
			wgpu_flags |= wgpu::BufferUsage::Vertex;
		}
		if (flags.Test(ResourceFlagBits::IndirectBuffer)) {
			wgpu_flags |= wgpu::BufferUsage::Indirect;
		}

		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::DeviceLocal, ResourceFlagBits::DeviceReadback);
		if (is_conflicting) {
			throw std::invalid_argument("DeviceLocal HostVisible or DeviceReadback are set simultaneously");
		}
	
		if (flags.Test(ResourceFlagBits::HostVisible)) {
			wgpu_flags |= wgpu::BufferUsage::MapWrite;
		}
		else if (flags.Test(ResourceFlagBits::DeviceReadback)) {
			wgpu_flags |= wgpu::BufferUsage::MapRead;
		}
		else {

		}
	
		return wgpu_flags;

	}

	wgpu::TextureUsage ExtractTextureUsage(ResourceFlags const& flags) noexcept {
		
		wgpu::TextureUsage wgpu_flags{};
	
		if (flags.Test(ResourceFlagBits::CopySRC)) {
			wgpu_flags |= wgpu::TextureUsage::CopySrc;
		}
		if (flags.Test(ResourceFlagBits::CopyDST)) {
			wgpu_flags |= wgpu::TextureUsage::CopyDst;
		}
		if (flags.Test(ResourceFlagBits::TextureBinding)) {
			wgpu_flags |= wgpu::TextureUsage::TextureBinding;
		}
		if (flags.Test(ResourceFlagBits::StorageBinding)) {
			wgpu_flags |= wgpu::TextureUsage::StorageBinding;
		}
		if (flags.Test(ResourceFlagBits::RenderAttachment)) {
			wgpu_flags |= wgpu::TextureUsage::RenderAttachment;
		}
		if (flags.Test(ResourceFlagBits::TransientAttachment)) {
			wgpu_flags |= wgpu::TextureUsage::TransientAttachment;
		}
		if (flags.Test(ResourceFlagBits::StorageAttachment)) {
			wgpu_flags |= wgpu::TextureUsage::StorageAttachment;
		}

		return wgpu_flags;

	}

	wgpu::TextureDimension ExtractTextureDimension(ResourceFlags const& flags) {
		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::Texture1D, ResourceFlagBits::Texture3D);
		if (is_conflicting) {
			throw std::invalid_argument("Texture1D Texture2D or Texture3D are set simultaneously");
		}
		if (flags.Test(ResourceFlagBits::Texture1D)) {
			return wgpu::TextureDimension::e1D;
		}
		else if (flags.Test(ResourceFlagBits::Texture2D)) {
			return wgpu::TextureDimension::e2D;
		}
		else if (flags.Test(ResourceFlagBits::Texture3D)) {
			return wgpu::TextureDimension::e3D;
		}
		else {
			return wgpu::TextureDimension::e2D;
		}
	}

	wgpu::TextureFormat ExtractFormat(ResourceFlags const& flags) {

		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::R8Unorm, ResourceFlagBits::Bc7UnormSrgb);
		if (is_conflicting) {
			throw std::invalid_argument("Only one format can be set");
		}
	
		// 8‑bit per component (1‑channel)
		if (flags.Test(ResourceFlagBits::R8Unorm)) return wgpu::TextureFormat::R8Unorm;
		else if (flags.Test(ResourceFlagBits::R8Snorm)) return wgpu::TextureFormat::R8Snorm;
		else if (flags.Test(ResourceFlagBits::R8Uint)) return wgpu::TextureFormat::R8Uint;
		else if (flags.Test(ResourceFlagBits::R8Sint)) return wgpu::TextureFormat::R8Sint;
	
		// 8‑bit per component (2‑channel)
		else if (flags.Test(ResourceFlagBits::R8G8Unorm)) return wgpu::TextureFormat::RG8Unorm;
		else if (flags.Test(ResourceFlagBits::R8G8Snorm)) return wgpu::TextureFormat::RG8Snorm;
		else if (flags.Test(ResourceFlagBits::R8G8Uint)) return wgpu::TextureFormat::RG8Uint;
		else if (flags.Test(ResourceFlagBits::R8G8Sint)) return wgpu::TextureFormat::RG8Sint;
	
		// 8‑bit per component (4‑channel)
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Unorm)) return wgpu::TextureFormat::RGBA8Unorm;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Snorm)) return wgpu::TextureFormat::RGBA8Snorm;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Uint)) return wgpu::TextureFormat::RGBA8Uint;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Sint)) return wgpu::TextureFormat::RGBA8Sint;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Srgb)) return wgpu::TextureFormat::RGBA8UnormSrgb;
		else if (flags.Test(ResourceFlagBits::B8G8R8A8Srgb)) return wgpu::TextureFormat::BGRA8UnormSrgb;
	
		// 16‑bit per component (1‑channel)
		else if (flags.Test(ResourceFlagBits::R16Unorm)) return wgpu::TextureFormat::R16Unorm;
		else if (flags.Test(ResourceFlagBits::R16Snorm)) return wgpu::TextureFormat::R16Snorm;
		else if (flags.Test(ResourceFlagBits::R16Uint)) return wgpu::TextureFormat::R16Uint;
		else if (flags.Test(ResourceFlagBits::R16Sint)) return wgpu::TextureFormat::R16Sint;
		else if (flags.Test(ResourceFlagBits::R16Float)) return wgpu::TextureFormat::R16Float;
	
		// 16‑bit per component (2‑channel)
		else if (flags.Test(ResourceFlagBits::R16G16Unorm)) return wgpu::TextureFormat::RG16Unorm;
		else if (flags.Test(ResourceFlagBits::R16G16Snorm)) return wgpu::TextureFormat::RG16Snorm;
		else if (flags.Test(ResourceFlagBits::R16G16Uint)) return wgpu::TextureFormat::RG16Uint;
		else if (flags.Test(ResourceFlagBits::R16G16Sint)) return wgpu::TextureFormat::RG16Sint;
		else if (flags.Test(ResourceFlagBits::R16G16Float)) return wgpu::TextureFormat::RG16Float;
	
		// 16‑bit per component (4‑channel)
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Unorm)) return wgpu::TextureFormat::RGBA16Unorm;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Snorm)) return wgpu::TextureFormat::RGBA16Snorm;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Uint)) return wgpu::TextureFormat::RGBA16Uint;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Sint)) return wgpu::TextureFormat::RGBA16Sint;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Float)) return wgpu::TextureFormat::RGBA16Float;
	
		// 32‑bit per component (1‑channel)
		else if (flags.Test(ResourceFlagBits::R32Uint)) return wgpu::TextureFormat::R32Uint;
		else if (flags.Test(ResourceFlagBits::R32Sint)) return wgpu::TextureFormat::R32Sint;
		else if (flags.Test(ResourceFlagBits::R32Float)) return wgpu::TextureFormat::R32Float;
	
		// 32‑bit per component (2‑channel)
		else if (flags.Test(ResourceFlagBits::R32G32Uint)) return wgpu::TextureFormat::RG32Uint;
		else if (flags.Test(ResourceFlagBits::R32G32Sint)) return wgpu::TextureFormat::RG32Sint;
		else if (flags.Test(ResourceFlagBits::R32G32Float)) return wgpu::TextureFormat::RG32Float;
	
		// 32‑bit per component (4‑channel)
		else if (flags.Test(ResourceFlagBits::R32G32B32A32Uint)) return wgpu::TextureFormat::RGBA32Uint;
		else if (flags.Test(ResourceFlagBits::R32G32B32A32Sint)) return wgpu::TextureFormat::RGBA32Sint;
		else if (flags.Test(ResourceFlagBits::R32G32B32A32Float)) return wgpu::TextureFormat::RGBA32Float;
	
		// Packed formats
		else if (flags.Test(ResourceFlagBits::R10G10B10A2Unorm)) return wgpu::TextureFormat::RGB10A2Unorm;
		else if (flags.Test(ResourceFlagBits::R10G10B10A2Uint)) return wgpu::TextureFormat::RGB10A2Uint;
		else if (flags.Test(ResourceFlagBits::R11G11B10Float)) return wgpu::TextureFormat::RG11B10Ufloat;
		else if (flags.Test(ResourceFlagBits::R9G9B9E5SharedExp)) return wgpu::TextureFormat::RGB9E5Ufloat;
	
		// Depth/stencil
		else if (flags.Test(ResourceFlagBits::D16Unorm)) return wgpu::TextureFormat::Depth16Unorm;
		else if (flags.Test(ResourceFlagBits::D24UnormS8Uint)) return wgpu::TextureFormat::Depth24PlusStencil8;
		else if (flags.Test(ResourceFlagBits::D32Float)) return wgpu::TextureFormat::Depth32Float;
		else if (flags.Test(ResourceFlagBits::D32FloatS8X24Uint)) return wgpu::TextureFormat::Depth32FloatStencil8;
	
		// BC compressed formats
		else if (flags.Test(ResourceFlagBits::Bc1Unorm)) return wgpu::TextureFormat::BC1RGBAUnorm;
		else if (flags.Test(ResourceFlagBits::Bc1UnormSrgb)) return wgpu::TextureFormat::BC1RGBAUnormSrgb;
		else if (flags.Test(ResourceFlagBits::Bc2Unorm)) return wgpu::TextureFormat::BC2RGBAUnorm;
		else if (flags.Test(ResourceFlagBits::Bc2UnormSrgb)) return wgpu::TextureFormat::BC2RGBAUnormSrgb;
		else if (flags.Test(ResourceFlagBits::Bc3Unorm)) return wgpu::TextureFormat::BC3RGBAUnorm;
		else if (flags.Test(ResourceFlagBits::Bc3UnormSrgb)) return wgpu::TextureFormat::BC3RGBAUnormSrgb;
		else if (flags.Test(ResourceFlagBits::Bc4Unorm)) return wgpu::TextureFormat::BC4RUnorm;
		else if (flags.Test(ResourceFlagBits::Bc4Snorm)) return wgpu::TextureFormat::BC4RSnorm;
		else if (flags.Test(ResourceFlagBits::Bc5Unorm)) return wgpu::TextureFormat::BC5RGUnorm;
		else if (flags.Test(ResourceFlagBits::Bc5Snorm)) return wgpu::TextureFormat::BC5RGSnorm;
		else if (flags.Test(ResourceFlagBits::Bc6HUfloat)) return wgpu::TextureFormat::BC6HRGBUfloat;
		else if (flags.Test(ResourceFlagBits::Bc6HSfloat)) return wgpu::TextureFormat::BC6HRGBFloat;
		else if (flags.Test(ResourceFlagBits::Bc7Unorm)) return wgpu::TextureFormat::BC7RGBAUnorm;
		else if (flags.Test(ResourceFlagBits::Bc7UnormSrgb)) return wgpu::TextureFormat::BC7RGBAUnormSrgb;
		
		else return wgpu::TextureFormat::Undefined;

	}

	std::uint32_t ExtractSampleCount(ResourceFlags const& flags) {
		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::Sample1, ResourceFlagBits::Sample64);
		if (is_conflicting) {
			throw std::invalid_argument("Only one sample count can be set");
		}
		if (flags.Test(ResourceFlagBits::Sample1)) return 1u;
		else if (flags.Test(ResourceFlagBits::Sample2)) return 2u;
		else if (flags.Test(ResourceFlagBits::Sample4)) return 4u;
		else if (flags.Test(ResourceFlagBits::Sample8)) return 8u;
		else if (flags.Test(ResourceFlagBits::Sample16)) return 16u;
		else if (flags.Test(ResourceFlagBits::Sample32)) return 32u;
		else if (flags.Test(ResourceFlagBits::Sample64)) return 64u;
		else return 1u;
	}

}

namespace fyuu_rhi::webgpu {
	Backend::Resource Backend::CreateBuffer(wgpu::Device const& ld, std::size_t size_in_bytes, ResourceFlags const& flags) {
		wgpu::BufferDescriptor desc{
			nullptr,
			{},
			ExtractBufferUsageFlags(flags),
			size_in_bytes,
			false
		};
		return { ld.CreateBuffer(&desc) };
	}

	Backend::Resource Backend::CreateTexture(wgpu::Device const& ld, std::size_t width, std::size_t height, std::size_t depth_arr_layers, std::size_t mip_lvl_cnt, ResourceFlags const& flags) {
		wgpu::TextureDescriptor desc{
			nullptr,
			{},
			ExtractTextureUsage(flags),
			ExtractTextureDimension(flags),
			{ static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), static_cast<std::uint32_t>(depth_arr_layers) },
			ExtractFormat(flags),
			static_cast<std::uint32_t>(mip_lvl_cnt),
			ExtractSampleCount(flags),
			0,
			nullptr
		};
		return { ld.CreateTexture(&desc) };
	}

}
