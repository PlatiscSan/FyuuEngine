module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <optional>
#include <variant>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include <D3D12MemAlloc.h>
module fyuu_rhi:d3d12_sampler_impl;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_sampler;
import :types;
import :d3d12_common;
import :d3d12_descriptor_allocator;
import :d3d12_logical_device;

namespace fyuu_rhi::d3d12 {

	D3D12Sampler::D3D12Sampler(D3D12LogicalDevice const& logical_device, SamplerFlags flags, SamplerAttachmentInfo const& attachment)
		: PolymorphicSamplerBase(this),
		D3D12Common(this),
		m_alloc(logical_device.GetSamplerAllocator()->Allocate()) {

		if (HasConflictingFlags(flags, SamplerFlags::FilterModes)) {
			throw std::invalid_argument("Multiple filter modes in sampler flags");
		}
		if (HasConflictingFlags(flags, SamplerFlags::AddressModes)) {
			throw std::invalid_argument("Multiple address modes in sampler flags");
		}
		if (HasConflictingFlags(flags, SamplerFlags::CompareFunctions)) {
			throw std::invalid_argument("Multiple compare functions in sampler flags");
		}
		if (HasConflictingFlags(flags, SamplerFlags::BorderColors)) {
			throw std::invalid_argument("Multiple border colors in sampler flags");
		}
		if (HasConflictingFlags(flags, SamplerFlags::ReductionModes)) {
			throw std::invalid_argument("Multiple reduction modes in sampler flags");
		}
		if (HasConflictingFlags(flags, SamplerFlags::AnisotropyModes)) {
			throw std::invalid_argument("Multiple anisotropy levels in sampler flags");
		}
		
		// Extract flag groups
		auto mag_filter = flags & SamplerFlags::MagMask;
		auto min_filter = flags & SamplerFlags::MinMask;
		auto mip_filter = flags & SamplerFlags::MipMask;
		auto address_mode = flags & SamplerFlags::AddressModes;
		auto compare_func = flags & SamplerFlags::CompareFunctions;
		auto border_color = flags & SamplerFlags::BorderColors;
		auto reduction_mode = flags & SamplerFlags::ReductionModes;
		auto anisotropy_mode = flags & SamplerFlags::AnisotropyModes;
		auto unnormalized = flags & SamplerFlags::UnnormalizedCoordinates;
		auto subsampled = flags & SamplerFlags::Subsampled;
		auto subsampled_coherent = flags & SamplerFlags::SubsampledCoherent;
		
		// Determine base filter type (point, linear, anisotropic)
		bool mag_linear = (flags & SamplerFlags::MagLinear) != SamplerFlags::Unknown;
		bool min_linear = (flags & SamplerFlags::MinLinear) != SamplerFlags::Unknown;
		bool mip_linear = (flags & SamplerFlags::MipLinear) != SamplerFlags::Unknown;
		bool any_linear = mag_linear || min_linear || mip_linear;
		
		bool anisotropic = (anisotropy_mode != SamplerFlags::AnisotropyOff && anisotropy_mode != SamplerFlags::Unknown);
		
		int base_filter = 0; // 0 = point, 1 = linear, 2 = anisotropic
		if (anisotropic)
			base_filter = 2;
		else if (any_linear)
			base_filter = 1;
		else
			base_filter = 0;
		
		// Apply reduction or comparison offset
		int filter_offset = 0;
		if (compare_func != SamplerFlags::Unknown)
			filter_offset = 16; // comparison
		else if (reduction_mode == SamplerFlags::Min)
			filter_offset = 48; // minimum
		else if (reduction_mode == SamplerFlags::Max)
			filter_offset = 64; // maximum
		// Average (standard) uses offset 0
		
		D3D12_SAMPLER_DESC desc = {};

		desc.Filter = static_cast<D3D12_FILTER>(base_filter + filter_offset);
		
		// Address modes (all axes same)
		if (address_mode == SamplerFlags::Repeat)
			desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		else if (address_mode == SamplerFlags::MirroredRepeat)
			desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		else if (address_mode == SamplerFlags::ClampToEdge)
			desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		else if (address_mode == SamplerFlags::ClampToBorder)
			desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		else if (address_mode == SamplerFlags::MirrorClampToEdge)
			desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		else
			desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // default
		
		// Mip LOD bias
		desc.MipLODBias = attachment.mip_lod_bias;
		
		// Max anisotropy
		if (anisotropy_mode == SamplerFlags::Anisotropy2x)
			desc.MaxAnisotropy = 2;
		else if (anisotropy_mode == SamplerFlags::Anisotropy4x)
			desc.MaxAnisotropy = 4;
		else if (anisotropy_mode == SamplerFlags::Anisotropy8x)
			desc.MaxAnisotropy = 8;
		else if (anisotropy_mode == SamplerFlags::Anisotropy16x)
			desc.MaxAnisotropy = 16;
		else
			desc.MaxAnisotropy = 1; // off
		
		// Comparison function
		if (compare_func == SamplerFlags::Never)
			desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		else if (compare_func == SamplerFlags::Less)
			desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
		else if (compare_func == SamplerFlags::Equal)
			desc.ComparisonFunc = D3D12_COMPARISON_FUNC_EQUAL;
		else if (compare_func == SamplerFlags::LessOrEqual)
			desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		else if (compare_func == SamplerFlags::Greater)
			desc.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER;
		else if (compare_func == SamplerFlags::NotEqual)
			desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
		else if (compare_func == SamplerFlags::GreaterOrEqual)
			desc.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		else if (compare_func == SamplerFlags::Always)
			desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		else
			desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // default when not used
		
		// Border color
		if (border_color == SamplerFlags::FloatTransparentBlack || border_color == SamplerFlags::IntTransparentBlack) {
			desc.BorderColor[0] = 0.0f; desc.BorderColor[1] = 0.0f; desc.BorderColor[2] = 0.0f; desc.BorderColor[3] = 0.0f;
		} else if (border_color == SamplerFlags::FloatOpaqueBlack || border_color == SamplerFlags::IntOpaqueBlack) {
			desc.BorderColor[0] = 0.0f; desc.BorderColor[1] = 0.0f; desc.BorderColor[2] = 0.0f; desc.BorderColor[3] = 1.0f;
		} else if (border_color == SamplerFlags::FloatOpaqueWhite || border_color == SamplerFlags::IntOpaqueWhite) {
			desc.BorderColor[0] = 1.0f; desc.BorderColor[1] = 1.0f; desc.BorderColor[2] = 1.0f; desc.BorderColor[3] = 1.0f;
		} else {
			// Default to transparent black if no border color specified
			desc.BorderColor[0] = 0.0f; desc.BorderColor[1] = 0.0f; desc.BorderColor[2] = 0.0f; desc.BorderColor[3] = 0.0f;
		}
		
		// Min/Max LOD
		desc.MinLOD = attachment.min_lod;
		desc.MaxLOD = attachment.max_lod;

		logical_device.CreateSampler(desc, m_alloc->cpu_handle);

	}

	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> D3D12Sampler::GetNative() const noexcept {
		return { m_alloc->cpu_handle, m_alloc->gpu_handle };
	}

}

#endif // defined(_WIN32)