/* vulkan_types.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <vector>
#include <optional>
#endif
export module fyuu_rhi:vulkan_types;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import vulkan;
import :enums;

namespace fyuu_rhi::vulkan {
	export struct VulkanSwapChainSupportDetails {
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> present_modes;
	};

	export struct VulkanCommandQueueInfo {
		CommandObjectType type;
		std::optional<std::uint32_t> family;
		std::uint32_t num_available;
	};

	export vk::ShaderStageFlags ShaderStageToVk(ShaderStage stage) noexcept {
		if (stage == ShaderStage::Unknown) return {};
		if (stage == ShaderStage::All) return vk::ShaderStageFlagBits::eAll;

		vk::ShaderStageFlags result;
		if ((stage & ShaderStage::Vertex) != ShaderStage::Unknown)
			result |= vk::ShaderStageFlagBits::eVertex;
		if ((stage & ShaderStage::Fragment) != ShaderStage::Unknown)
			result |= vk::ShaderStageFlagBits::eFragment;
		if ((stage & ShaderStage::Compute) != ShaderStage::Unknown)
			result |= vk::ShaderStageFlagBits::eCompute;
		if ((stage & ShaderStage::Geometry) != ShaderStage::Unknown)
			result |= vk::ShaderStageFlagBits::eGeometry;
		if ((stage & ShaderStage::TessellationControl) != ShaderStage::Unknown)
			result |= vk::ShaderStageFlagBits::eTessellationControl;
		if ((stage & ShaderStage::TessellationEvaluation) != ShaderStage::Unknown)
			result |= vk::ShaderStageFlagBits::eTessellationEvaluation;
		if ((stage & ShaderStage::Amplification) != ShaderStage::Unknown)
			result |= vk::ShaderStageFlagBits::eTaskEXT;
		if ((stage & ShaderStage::Mesh) != ShaderStage::Unknown)
			result |= vk::ShaderStageFlagBits::eMeshEXT;
		if ((stage & ShaderStage::RayGeneration) != ShaderStage::Unknown)
			result |= vk::ShaderStageFlagBits::eRaygenKHR;
		if ((stage & ShaderStage::RayIntersection) != ShaderStage::Unknown)
			result |= vk::ShaderStageFlagBits::eIntersectionKHR;
		if ((stage & ShaderStage::RayAnyHit) != ShaderStage::Unknown)
			result |= vk::ShaderStageFlagBits::eAnyHitKHR;
		if ((stage & ShaderStage::RayClosestHit) != ShaderStage::Unknown)
			result |= vk::ShaderStageFlagBits::eClosestHitKHR;
		if ((stage & ShaderStage::RayMiss) != ShaderStage::Unknown)
			result |= vk::ShaderStageFlagBits::eMissKHR;
		if ((stage & ShaderStage::Callable) != ShaderStage::Unknown)
			result |= vk::ShaderStageFlagBits::eCallableKHR;
		return result;
	}

	export vk::ShaderStageFlagBits ShaderStageToVkBits(ShaderStage stage) noexcept {
		switch (stage) {
		case ShaderStage::Vertex: return vk::ShaderStageFlagBits::eVertex;
		case ShaderStage::Fragment: return vk::ShaderStageFlagBits::eFragment;
		case ShaderStage::Compute: return vk::ShaderStageFlagBits::eCompute;
		case ShaderStage::Geometry: return vk::ShaderStageFlagBits::eGeometry;
		case ShaderStage::TessellationControl: return vk::ShaderStageFlagBits::eTessellationControl;
		case ShaderStage::TessellationEvaluation: return vk::ShaderStageFlagBits::eTessellationEvaluation;
		case ShaderStage::Amplification: return vk::ShaderStageFlagBits::eTaskEXT;
		case ShaderStage::Mesh: return vk::ShaderStageFlagBits::eMeshEXT;
		case ShaderStage::RayGeneration: return vk::ShaderStageFlagBits::eRaygenKHR;
		case ShaderStage::RayIntersection: return vk::ShaderStageFlagBits::eIntersectionKHR;
		case ShaderStage::RayAnyHit: return vk::ShaderStageFlagBits::eAnyHitKHR;
		case ShaderStage::RayClosestHit: return vk::ShaderStageFlagBits::eClosestHitKHR;
		case ShaderStage::RayMiss: return vk::ShaderStageFlagBits::eMissKHR;
		case ShaderStage::Callable: return vk::ShaderStageFlagBits::eCallableKHR;
		default: 
			return {};
		}
	}

	export vk::Format DetermineFormat(ResourceFlags flags) noexcept {

		if (HasConflictingFlags(flags, ResourceFlags::AllFormats)) {
			return vk::Format::eUndefined;
		}
	
		if ((flags & ResourceFlags::BGRA8_Unorm) != ResourceFlags::Unknown)
			return vk::Format::eB8G8R8A8Unorm;
		if ((flags & ResourceFlags::RGBA8_Uint) != ResourceFlags::Unknown)
			return vk::Format::eR8G8B8A8Uint;
		if ((flags & ResourceFlags::RGBA8_Unorm) != ResourceFlags::Unknown)
			return vk::Format::eR8G8B8A8Unorm;
		if ((flags & ResourceFlags::RGBA16_Unorm) != ResourceFlags::Unknown)
			return vk::Format::eR16G16B16A16Unorm;
		if ((flags & ResourceFlags::RGBA16_Snorm) != ResourceFlags::Unknown)
			return vk::Format::eR16G16B16A16Snorm;
		if ((flags & ResourceFlags::RGBA16_Sfloat) != ResourceFlags::Unknown)
			return vk::Format::eR16G16B16A16Sfloat;
		if ((flags & ResourceFlags::RGBA32_Sfloat) != ResourceFlags::Unknown)
			return vk::Format::eR32G32B32A32Sfloat;
		if ((flags & ResourceFlags::R8_Uint) != ResourceFlags::Unknown)
			return vk::Format::eR8Uint;
		if ((flags & ResourceFlags::R16_Float) != ResourceFlags::Unknown)
			return vk::Format::eR16Sfloat;
		if ((flags & ResourceFlags::R32_Uint) != ResourceFlags::Unknown)
			return vk::Format::eR32Uint;
		if ((flags & ResourceFlags::R32_Float) != ResourceFlags::Unknown)
			return vk::Format::eR32Sfloat;
		if ((flags & ResourceFlags::BC1_RGB_Unorm) != ResourceFlags::Unknown)
			return vk::Format::eBc1RgbUnormBlock;
		if ((flags & ResourceFlags::BC1_RGB_SRGB) != ResourceFlags::Unknown)
			return vk::Format::eBc1RgbSrgbBlock;
		if ((flags & ResourceFlags::BC1_RGBA_Unorm) != ResourceFlags::Unknown)
			return vk::Format::eBc1RgbaUnormBlock;
		if ((flags & ResourceFlags::BC1_RGBA_SRGB) != ResourceFlags::Unknown)
			return vk::Format::eBc1RgbaSrgbBlock;
		if ((flags & ResourceFlags::BC2_Unorm) != ResourceFlags::Unknown)
			return vk::Format::eBc2UnormBlock;
		if ((flags & ResourceFlags::BC2_SRGB) != ResourceFlags::Unknown)
			return vk::Format::eBc2SrgbBlock;
		if ((flags & ResourceFlags::BC3_Unorm) != ResourceFlags::Unknown)
			return vk::Format::eBc3UnormBlock;
		if ((flags & ResourceFlags::BC3_SRGB) != ResourceFlags::Unknown)
			return vk::Format::eBc3SrgbBlock;
		if ((flags & ResourceFlags::BC4_Unorm) != ResourceFlags::Unknown)
			return vk::Format::eBc4UnormBlock;
		if ((flags & ResourceFlags::BC4_SRGB) != ResourceFlags::Unknown)
			return vk::Format::eBc4UnormBlock;          // No SRGB variant for BC4 in Vulkan
		if ((flags & ResourceFlags::BC5_Unorm) != ResourceFlags::Unknown)
			return vk::Format::eBc5UnormBlock;
		if ((flags & ResourceFlags::BC5_SRGB) != ResourceFlags::Unknown)
			return vk::Format::eBc5UnormBlock;          // No SRGB variant for BC5 in Vulkan
		if ((flags & ResourceFlags::D32SFloat) != ResourceFlags::Unknown)
			return vk::Format::eD32Sfloat;
		if ((flags & ResourceFlags::D24UnormS8Uint) != ResourceFlags::Unknown)
			return vk::Format::eD24UnormS8Uint;
	
		// New formats
		if ((flags & ResourceFlags::R8_Unorm) != ResourceFlags::Unknown)
			return vk::Format::eR8Unorm;
		if ((flags & ResourceFlags::R8_Snorm) != ResourceFlags::Unknown)
			return vk::Format::eR8Snorm;
		if ((flags & ResourceFlags::RG8_Unorm) != ResourceFlags::Unknown)
			return vk::Format::eR8G8Unorm;
		if ((flags & ResourceFlags::RG8_Snorm) != ResourceFlags::Unknown)
			return vk::Format::eR8G8Snorm;
		if ((flags & ResourceFlags::R16_Unorm) != ResourceFlags::Unknown)
			return vk::Format::eR16Unorm;
		if ((flags & ResourceFlags::R16_Snorm) != ResourceFlags::Unknown)
			return vk::Format::eR16Snorm;
		if ((flags & ResourceFlags::RG16_Sfloat) != ResourceFlags::Unknown)
			return vk::Format::eR16G16Sfloat;
		if ((flags & ResourceFlags::R32_Sint) != ResourceFlags::Unknown)
			return vk::Format::eR32Sint;
		if ((flags & ResourceFlags::RG32_Sfloat) != ResourceFlags::Unknown)
			return vk::Format::eR32G32Sfloat;
		if ((flags & ResourceFlags::RGB10A2_Unorm) != ResourceFlags::Unknown)
			return vk::Format::eA2B10G10R10UnormPack32;   // Corresponds to R10G10B10A2_UNORM
		if ((flags & ResourceFlags::R11G11B10_UFloat) != ResourceFlags::Unknown)
			return vk::Format::eB10G11R11UfloatPack32;    // Corresponds to R11G11B10_FLOAT
		if ((flags & ResourceFlags::RGBA8_SRGB) != ResourceFlags::Unknown)
			return vk::Format::eR8G8B8A8Srgb;
		if ((flags & ResourceFlags::BGRA8_SRGB) != ResourceFlags::Unknown)
			return vk::Format::eB8G8R8A8Srgb;
		if ((flags & ResourceFlags::RGBA8_Snorm) != ResourceFlags::Unknown)
			return vk::Format::eR8G8B8A8Snorm;
		if ((flags & ResourceFlags::RGBA8_Sint) != ResourceFlags::Unknown)
			return vk::Format::eR8G8B8A8Sint;
		if ((flags & ResourceFlags::R16_Uint) != ResourceFlags::Unknown)
			return vk::Format::eR16Uint;
		if ((flags & ResourceFlags::D16_Unorm) != ResourceFlags::Unknown)
			return vk::Format::eD16Unorm;
		if ((flags & ResourceFlags::D32FloatS8Uint) != ResourceFlags::Unknown)
			return vk::Format::eD32SfloatS8Uint;
		if ((flags & ResourceFlags::BC6H_UF16) != ResourceFlags::Unknown)
			return vk::Format::eBc6HUfloatBlock;
		if ((flags & ResourceFlags::BC6H_SF16) != ResourceFlags::Unknown)
			return vk::Format::eBc6HSfloatBlock;
		if ((flags & ResourceFlags::BC7_Unorm) != ResourceFlags::Unknown)
			return vk::Format::eBc7UnormBlock;
		if ((flags & ResourceFlags::BC7_SRGB) != ResourceFlags::Unknown)
			return vk::Format::eBc7SrgbBlock;
	
		return vk::Format::eUndefined;
	}

	export std::pair<vk::Format, vk::Format> ToDepthStencilFormat(ResourceFlags format) {

		if (HasConflictingFlags(format, ResourceFlags::AllFormats)) {
			return { vk::Format::eUndefined, vk::Format::eUndefined };
		}

		switch (format) {
		case ResourceFlags::D32SFloat:
			return {vk::Format::eD32Sfloat, vk::Format::eUndefined};
		case ResourceFlags::D16_Unorm:
			return {vk::Format::eD16Unorm, vk::Format::eUndefined};
		case ResourceFlags::D24UnormS8Uint:
			// Depth part: 24‑bit unorm + 8 unused bits
			// Stencil part: pure stencil
			return {vk::Format::eX8D24UnormPack32, vk::Format::eS8Uint};
		case ResourceFlags::D32FloatS8Uint:
			// Depth part: 32‑bit float, stencil part: pure stencil
			return {vk::Format::eD32Sfloat, vk::Format::eS8Uint};
		default:
			// Invalid or unsupported depth-stencil format.
			// In a real implementation you might assert or throw.
			return {vk::Format::eUndefined, vk::Format::eUndefined};
		}
	}

}
#endif // !defined(__APPLE__)

