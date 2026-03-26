/* d3d12_shader_data_segment.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
#include <variant>
#include <optional>
#include <concepts>
#include <format>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "declare_pool.hpp"
module fyuu_rhi:d3d12_shader_data_segment_impl;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_shader_data_segment;
import plastic.registrable;
import :enums;
import :types;
import :d3d12_common;
import :d3d12_logical_device;
import :d3d12_throw;

namespace {
	using namespace fyuu_rhi;
	using namespace fyuu_rhi::d3d12;

	std::optional<D3D12_DESCRIPTOR_RANGE_TYPE> DescriptorRangeTypeFromResourceFlags(ResourceFlags flags) noexcept {

		if (HasConflictingFlags(flags, ResourceFlags::Buffer, ResourceFlags::Texture)) {
			return std::nullopt;
		}

		if ((flags & ResourceFlags::UniformBuffer) != ResourceFlags::Unknown) {
			return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		}

		if ((flags & ResourceFlags::UniformTexelBuffer) != ResourceFlags::Unknown || (flags & ResourceFlags::SampledTexture) != ResourceFlags::Unknown) {
			return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		}

		if ((flags & ResourceFlags::StorageTexelBuffer) != ResourceFlags::Unknown ||
			(flags & ResourceFlags::StorageBuffer) != ResourceFlags::Unknown ||
			(flags & ResourceFlags::StorageTexture) != ResourceFlags::Unknown) {
			return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		}

		return std::nullopt;

	}

	D3D12_SHADER_VISIBILITY VisibilityFromShaderStage(ShaderStage stage) noexcept {

		if (HasConflictingFlags(stage, ShaderStage::All)) {
			return D3D12_SHADER_VISIBILITY_ALL;
		}

		switch (stage) {
		case ShaderStage::Vertex:
			return D3D12_SHADER_VISIBILITY_VERTEX;
		case ShaderStage::Fragment:
			return D3D12_SHADER_VISIBILITY_PIXEL;
		case ShaderStage::Geometry:
				return D3D12_SHADER_VISIBILITY_GEOMETRY;
		case ShaderStage::TessellationControl:
			return D3D12_SHADER_VISIBILITY_HULL;
		case ShaderStage::TessellationEvaluation:
			return D3D12_SHADER_VISIBILITY_DOMAIN;
		default:
			return D3D12_SHADER_VISIBILITY_ALL;
		}

	}

	D3D12_STATIC_SAMPLER_DESC CreateSamplerDescription(SamplerFlags flags, SamplerAttachmentInfo const& attachment, std::size_t where, D3D12_SHADER_VISIBILITY visibility, std::size_t ns = 0u) {
		
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
	
		D3D12_STATIC_SAMPLER_DESC desc = {};
		desc.ShaderRegister = static_cast<UINT>(where);
		desc.RegisterSpace = static_cast<UINT>(ns);
		desc.ShaderVisibility = visibility;
	
		desc.MipLODBias = attachment.mip_lod_bias;
		desc.MinLOD = attachment.min_lod;
		desc.MaxLOD = attachment.max_lod;
	
		if ((flags & SamplerFlags::Repeat) != SamplerFlags::Unknown) {
			desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		} else if ((flags & SamplerFlags::MirroredRepeat) != SamplerFlags::Unknown) {
			desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		} else if ((flags & SamplerFlags::ClampToEdge) != SamplerFlags::Unknown) {
			desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		} else if ((flags & SamplerFlags::ClampToBorder) != SamplerFlags::Unknown) {
			desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		} else if ((flags & SamplerFlags::MirrorClampToEdge) != SamplerFlags::Unknown) {
			desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		} else {
			desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		}
	
		if ((flags & SamplerFlags::Never) != SamplerFlags::Unknown)          desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		else if ((flags & SamplerFlags::Less) != SamplerFlags::Unknown)      desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
		else if ((flags & SamplerFlags::Equal) != SamplerFlags::Unknown)     desc.ComparisonFunc = D3D12_COMPARISON_FUNC_EQUAL;
		else if ((flags & SamplerFlags::LessOrEqual) != SamplerFlags::Unknown) desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		else if ((flags & SamplerFlags::Greater) != SamplerFlags::Unknown)   desc.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER;
		else if ((flags & SamplerFlags::NotEqual) != SamplerFlags::Unknown)  desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
		else if ((flags & SamplerFlags::GreaterOrEqual) != SamplerFlags::Unknown) desc.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		else if ((flags & SamplerFlags::Always) != SamplerFlags::Unknown)    desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		else desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	
		if ((flags & SamplerFlags::FloatTransparentBlack) != SamplerFlags::Unknown) {
			desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		} else if ((flags & SamplerFlags::FloatOpaqueBlack) != SamplerFlags::Unknown) {
			desc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		} else if ((flags & SamplerFlags::FloatOpaqueWhite) != SamplerFlags::Unknown) {
			desc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		} else if ((flags & SamplerFlags::IntTransparentBlack) != SamplerFlags::Unknown) {
			desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		} else if ((flags & SamplerFlags::IntOpaqueBlack) != SamplerFlags::Unknown) {
			desc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		} else if ((flags & SamplerFlags::IntOpaqueWhite) != SamplerFlags::Unknown) {
			desc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		} else {
			desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		}
		
		if ((flags & SamplerFlags::AnisotropyOff) != SamplerFlags::Unknown) desc.MaxAnisotropy = 1;
		else if ((flags & SamplerFlags::Anisotropy2x) != SamplerFlags::Unknown) desc.MaxAnisotropy = 2;
		else if ((flags & SamplerFlags::Anisotropy4x) != SamplerFlags::Unknown) desc.MaxAnisotropy = 4;
		else if ((flags & SamplerFlags::Anisotropy8x) != SamplerFlags::Unknown) desc.MaxAnisotropy = 8;
		else if ((flags & SamplerFlags::Anisotropy16x) != SamplerFlags::Unknown) desc.MaxAnisotropy = 16;
		else desc.MaxAnisotropy = 1;
	
		bool comparison = (flags & SamplerFlags::CompareFunctions) != SamplerFlags::Unknown;
		bool reduction = (flags & SamplerFlags::ReductionModes) != SamplerFlags::Unknown;
		D3D12_FILTER_TYPE mag = D3D12_FILTER_TYPE_LINEAR, min = D3D12_FILTER_TYPE_LINEAR, mip = D3D12_FILTER_TYPE_LINEAR;
	
		if ((flags & SamplerFlags::MagNearest) != SamplerFlags::Unknown) mag = D3D12_FILTER_TYPE_POINT;
		if ((flags & SamplerFlags::MinNearest) != SamplerFlags::Unknown) min = D3D12_FILTER_TYPE_POINT;
		if ((flags & SamplerFlags::MipNearest) != SamplerFlags::Unknown) mip = D3D12_FILTER_TYPE_POINT;
	
		if (desc.MaxAnisotropy > 1) {
			if (comparison) desc.Filter = D3D12_FILTER_COMPARISON_ANISOTROPIC;
			else if (reduction) {
				if ((flags & SamplerFlags::Min) != SamplerFlags::Unknown) desc.Filter = D3D12_FILTER_MINIMUM_ANISOTROPIC;
				else if ((flags & SamplerFlags::Max) != SamplerFlags::Unknown) desc.Filter = D3D12_FILTER_MAXIMUM_ANISOTROPIC;
				else /* Average */ desc.Filter = D3D12_FILTER_ANISOTROPIC;
			} else desc.Filter = D3D12_FILTER_ANISOTROPIC;
		} else {
			if (comparison) {
				desc.Filter = D3D12_ENCODE_BASIC_FILTER(mag, min, mip, D3D12_FILTER_REDUCTION_TYPE_COMPARISON);
			} else if (reduction) {
				D3D12_FILTER_REDUCTION_TYPE reductionType = D3D12_FILTER_REDUCTION_TYPE_STANDARD;
				if ((flags & SamplerFlags::Min) != SamplerFlags::Unknown) reductionType = D3D12_FILTER_REDUCTION_TYPE_MINIMUM;
				else if ((flags & SamplerFlags::Max) != SamplerFlags::Unknown) reductionType = D3D12_FILTER_REDUCTION_TYPE_MAXIMUM;
				desc.Filter = D3D12_ENCODE_BASIC_FILTER(mag, min, mip, reductionType);
			} else {
				desc.Filter = D3D12_ENCODE_BASIC_FILTER(mag, min, mip, D3D12_FILTER_REDUCTION_TYPE_STANDARD);
			}
		}
		
		return desc;
	}
}

namespace fyuu_rhi::d3d12 {

	RootSignatureReflector::RootSignatureReflector(std::span<std::byte const> bytes)
		: m_deserializer(
			[bytes]() {
				Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> deserializer;
				ThrowIfFailed(
					D3D12CreateVersionedRootSignatureDeserializer(
						bytes.data(),
						static_cast<SIZE_T>(bytes.size()),
						IID_PPV_ARGS(&deserializer)
					)
				);
				return deserializer;
			}()) {
	}

	std::optional<std::size_t> RootSignatureReflector::IndexOf(std::size_t ns, std::size_t where, Type type) const {
		
	    auto* versioned_desc = m_deserializer->GetUnconvertedRootSignatureDesc();
		if (!versioned_desc) {
			return std::nullopt;
		}

		auto range_matches = [&](UINT base_reg, UINT num_regs, UINT space, D3D12_DESCRIPTOR_RANGE_TYPE range_type) -> bool {
			if (space != ns) return false;
			if (where < base_reg || where >= base_reg + num_regs) return false;
			switch (type) {
			case Type::CBV: return range_type == D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			case Type::UAV: return range_type == D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			case Type::SRV: return range_type == D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			case Type::Sampler: return range_type == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			default: return false;
			}
		};

		auto descriptor_matches = [&](UINT shader_reg, UINT reg_space, D3D12_ROOT_PARAMETER_TYPE param_type) -> bool {
			if (shader_reg != where || reg_space != ns) return false;
			switch (type) {
			case Type::CBV: return param_type == D3D12_ROOT_PARAMETER_TYPE_CBV;
			case Type::SRV: return param_type == D3D12_ROOT_PARAMETER_TYPE_SRV;
			case Type::UAV: return param_type == D3D12_ROOT_PARAMETER_TYPE_UAV;
			default: return false;
			}
		};

		auto constants_matches = [&](UINT shader_reg, UINT reg_space) -> bool {
			return type == Type::RootConstant && shader_reg == where && reg_space == ns;
		};

		switch (versioned_desc->Version) {
		case D3D_ROOT_SIGNATURE_VERSION_1_0: {
			const auto& desc = versioned_desc->Desc_1_0;
			for (UINT i = 0; i < desc.NumParameters; ++i) {
				const auto& param = desc.pParameters[i];
				switch (param.ParameterType) {
				case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
					for (UINT j = 0; j < param.DescriptorTable.NumDescriptorRanges; ++j) {
						const auto& range = param.DescriptorTable.pDescriptorRanges[j];
						if (range_matches(range.BaseShaderRegister, range.NumDescriptors, range.RegisterSpace, range.RangeType)) {
							return i;
						}
					}
					break;
				case D3D12_ROOT_PARAMETER_TYPE_CBV:
				case D3D12_ROOT_PARAMETER_TYPE_SRV:
				case D3D12_ROOT_PARAMETER_TYPE_UAV:
					if (descriptor_matches(param.Descriptor.ShaderRegister, param.Descriptor.RegisterSpace, param.ParameterType)) {
						return i;
					}
					break;
				case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
					if (constants_matches(param.Constants.ShaderRegister, param.Constants.RegisterSpace)) {
						return i;
					}
					break;
				default:
					break;
				}
			}
			break;
		}

		case D3D_ROOT_SIGNATURE_VERSION_1_1: {
			auto const& desc = versioned_desc->Desc_1_1;
			for (UINT i = 0; i < desc.NumParameters; ++i) {
				const auto& param = desc.pParameters[i];
				switch (param.ParameterType) {
				case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
					for (UINT j = 0; j < param.DescriptorTable.NumDescriptorRanges; ++j) {
						const auto& range = param.DescriptorTable.pDescriptorRanges[j];
						if (range_matches(range.BaseShaderRegister, range.NumDescriptors, range.RegisterSpace, range.RangeType)) {
							return i;
						}
					}
					break;
				case D3D12_ROOT_PARAMETER_TYPE_CBV:
				case D3D12_ROOT_PARAMETER_TYPE_SRV:
				case D3D12_ROOT_PARAMETER_TYPE_UAV:
					if (descriptor_matches(param.Descriptor.ShaderRegister, param.Descriptor.RegisterSpace, param.ParameterType)) {
						return i;
					}
					break;
				case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
					if (constants_matches(param.Constants.ShaderRegister, param.Constants.RegisterSpace)) {
						return i;
					}
					break;
				default:
					break;
				}
			}
			break;
		}

		default:
			return std::nullopt;
		}
	
	}

	D3D12ShaderDataSegment::D3D12ShaderDataSegment(D3D12LogicalDevice const& logical_device)
		: PolymorphicShaderDataSegmentBase(this),
		D3D12Common(this),
		m_logical_device_id(logical_device.GetID()),
		m_handle() {

	}

#define VALIDATE_STATE_AND_STAGE \
		auto* declarations = std::get_if<std::vector<Declaration>>(&m_handle);	\
		if (!declarations) {	\
			throw std::logic_error("Cannot declare after instantiation");	\
		}	\
		if (visible == ShaderStage::Unknown) {	\
			throw std::invalid_argument("Visible stage cannot be Unknown");	\
		}

	D3D12ShaderDataSegment& D3D12ShaderDataSegment::Declare(std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns) {

		VALIDATE_STATE_AND_STAGE
		
		D3D12_SHADER_VISIBILITY visibility = VisibilityFromShaderStage(visible);
		declarations->push_back({ where, ns, Declaration::RootConstantInfo{ count }, visibility });

		return *this;

	}

	D3D12ShaderDataSegment& D3D12ShaderDataSegment::Declare(ResourceFlags flags, std::size_t where, ShaderStage visible, std::size_t ns) {

		VALIDATE_STATE_AND_STAGE

		D3D12_SHADER_VISIBILITY visibility = VisibilityFromShaderStage(visible);

		std::optional<D3D12_DESCRIPTOR_RANGE_TYPE> range_type = DescriptorRangeTypeFromResourceFlags(flags);
		if (!range_type) {
			throw std::invalid_argument("invalid resource flags");
		}

		declarations->push_back({ where, ns, Declaration::RootDescriptorInfo{ *range_type }, visibility });

		return *this;

	}

	D3D12ShaderDataSegment& D3D12ShaderDataSegment::Declare(ResourceFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns) {
		
		VALIDATE_STATE_AND_STAGE

		D3D12_SHADER_VISIBILITY visibility = VisibilityFromShaderStage(visible);

		std::optional<D3D12_DESCRIPTOR_RANGE_TYPE> range_type = DescriptorRangeTypeFromResourceFlags(flags);
		if (!range_type) {
			throw std::invalid_argument("invalid resource flags");
		}
		
		declarations->push_back({ where, ns, Declaration::DescriptorTableInfo{ count, *range_type }, visibility });

		return *this;

	}

	D3D12ShaderDataSegment& D3D12ShaderDataSegment::Declare(SamplerFlags flags, SamplerAttachmentInfo const& info, std::size_t where, ShaderStage visible, std::size_t ns) {

		VALIDATE_STATE_AND_STAGE

		D3D12_SHADER_VISIBILITY visibility = VisibilityFromShaderStage(visible);
		declarations->push_back({ where, ns, Declaration::StaticSamplerInfo{ flags, info }, visibility });

		return *this;

	}

	D3D12ShaderDataSegment& D3D12ShaderDataSegment::Declare(SamplerFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns) {

		VALIDATE_STATE_AND_STAGE
		D3D12_SHADER_VISIBILITY visibility = VisibilityFromShaderStage(visible);

		declarations->push_back({ where, ns, Declaration::DescriptorTableInfo{ count, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER }, visibility });

		return *this;

	}

	void D3D12ShaderDataSegment::Instantiate() {

		auto* declarations = std::get_if<std::vector<Declaration>>(&m_handle);
		if (!declarations) {
			throw std::logic_error("Cannot instantiate again after instantiation");
		}	

		std::vector<CD3DX12_ROOT_PARAMETER> root_parameters;
		std::vector<CD3DX12_DESCRIPTOR_RANGE> ranges;
		std::vector<D3D12_STATIC_SAMPLER_DESC> static_samplers;

		for (auto const& declaration : *declarations) {
			std::visit(
				[&declaration, &root_parameters, &ranges, &static_samplers](auto&& info) {
					using Info = std::decay_t<decltype(info)>;
					if constexpr (std::same_as<Info, Declaration::DescriptorTableInfo>) {
						CD3DX12_DESCRIPTOR_RANGE& range = ranges.emplace_back();
						range.Init(
							info.range_type,
							static_cast<UINT>(info.count),
							static_cast<UINT>(declaration.where),
							declaration.ns,
							D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
						);
						root_parameters.emplace_back().InitAsDescriptorTable(
							1u, &range,
							declaration.visibility
						);
					}
					else if constexpr (std::same_as<Info, Declaration::RootDescriptorInfo>) {
						switch (info.range_type) {
						case D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
							root_parameters.emplace_back().InitAsShaderResourceView(
								declaration.where,
								declaration.ns,
								declaration.visibility
							);
							break;
						case D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
							root_parameters.emplace_back().InitAsUnorderedAccessView(
								declaration.where,
								declaration.ns,
								declaration.visibility
							);
							break;		
						case D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
							root_parameters.emplace_back().InitAsConstantBufferView(
								declaration.where,
								declaration.ns,
								declaration.visibility
							);
							break;
						default:
							break;
						}
					}
					else if constexpr (std::same_as<Info, Declaration::RootConstantInfo>) {
						root_parameters.emplace_back().InitAsConstants(
							info.count,
							declaration.where,
							declaration.ns,
							declaration.visibility
						);
					}
					else if constexpr (std::same_as<Info, Declaration::StaticSamplerInfo>){
						static_samplers.emplace_back(CreateSamplerDescription(info.flags, info.attachment, declaration.where, declaration.visibility, declaration.ns));
					}
					else {

					}
				},
				declaration.info
			);
		}

		CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc(
			static_cast<UINT>(root_parameters.size()),
			root_parameters.data(),
			static_cast<UINT>(static_samplers.size()),
			static_samplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		Microsoft::WRL::ComPtr<ID3DBlob> signature_blob;
		Microsoft::WRL::ComPtr<ID3DBlob> error_blob;
		HRESULT hr = D3D12SerializeRootSignature(
			&root_signature_desc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&signature_blob,
			&error_blob
		);

		if (FAILED(hr)) {
			throw std::runtime_error(std::format("Failed to serialize root signature, {}", static_cast<char const*>(error_blob->GetBufferPointer())));
		}

		if (!m_logical_device_id) {
			throw std::runtime_error("Invalid logical device"); 
		}

		auto logical_device = plastic::utility::QueryObject<D3D12LogicalDevice>(*m_logical_device_id); 
			
		if (!logical_device) {
			throw std::runtime_error("logical device lost");
		}

		m_handle.emplace<InstantiatedData>(
			logical_device->CreateRootSignature(signature_blob.Get()),
			std::span<std::byte const>(reinterpret_cast<std::byte const*>(signature_blob->GetBufferPointer()), signature_blob->GetBufferSize())
		);

	}
	
	void D3D12ShaderDataSegment::Reset() {
		m_handle.emplace<std::vector<Declaration>>();
	}

	RootSignatureReflector const* D3D12ShaderDataSegment::GetReflector() const noexcept {
		if (auto const* instantiated = std::get_if<InstantiatedData>(&m_handle)) {
			return &instantiated->reflector;
		}
		return nullptr;
	}

	ID3D12RootSignature* D3D12ShaderDataSegment::GetRawInstantiatedNative() const noexcept {
		if (auto const* instantiated = std::get_if<InstantiatedData>(&m_handle)) {
			return instantiated->root_signature.Get();
		}
		return nullptr;
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> D3D12ShaderDataSegment::GetInstantiatedNative() const noexcept {
		if (auto const* instantiated = std::get_if<InstantiatedData>(&m_handle)) {
			return instantiated->root_signature;
		}
		return nullptr;
	}

} // namespace fyuu_rhi::d3d12

#endif // defined(_WIN32)