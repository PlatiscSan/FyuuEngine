/* d3d12_gpu_executable.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <algorithm>
#include <utility>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <variant>
#include <optional>
#include <span>
#include <format>
#include <ranges>
#endif // !defined(__cpp_lib_modules)
#include "declare_pool.hpp"
#include "platform.hpp"
#include <d3dcompiler.h>
#include <dxcapi.h>
module fyuu_rhi:d3d12_gpu_executable_impl;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_gpu_executable;
import plastic.registrable;
import :enums;
import :types;
import :d3d12_common;
import :d3d12_logical_device;
import :d3d12_shader;
import :d3d12_shader_data_segment;
import :d3d12_types;

namespace {
	using namespace fyuu_rhi;
	using namespace fyuu_rhi::d3d12;

	 
	D3D12_BLEND ToD3D12BlendFactor(BlendFactor factor) noexcept {
		switch (factor) {
		case BlendFactor::Zero:				  return D3D12_BLEND_ZERO;
		case BlendFactor::One:				   return D3D12_BLEND_ONE;
		case BlendFactor::SrcColor:			  return D3D12_BLEND_SRC_COLOR;
		case BlendFactor::OneMinusSrcColor:	  return D3D12_BLEND_INV_SRC_COLOR;
		case BlendFactor::DstColor:			  return D3D12_BLEND_DEST_COLOR;
		case BlendFactor::OneMinusDstColor:	  return D3D12_BLEND_INV_DEST_COLOR;
		case BlendFactor::SrcAlpha:			   return D3D12_BLEND_SRC_ALPHA;
		case BlendFactor::OneMinusSrcAlpha:	   return D3D12_BLEND_INV_SRC_ALPHA;
		case BlendFactor::DstAlpha:			   return D3D12_BLEND_DEST_ALPHA;
		case BlendFactor::OneMinusDstAlpha:	   return D3D12_BLEND_INV_DEST_ALPHA;
		case BlendFactor::ConstantColor:		  return D3D12_BLEND_BLEND_FACTOR;
		case BlendFactor::OneMinusConstantColor:  return D3D12_BLEND_INV_BLEND_FACTOR;
		case BlendFactor::ConstantAlpha:		  return D3D12_BLEND_ALPHA_FACTOR;
		case BlendFactor::OneMinusConstantAlpha:  return D3D12_BLEND_INV_ALPHA_FACTOR;
		case BlendFactor::SrcAlphaSaturate:	   return D3D12_BLEND_SRC_ALPHA_SAT;
		case BlendFactor::Src1Color:			  return D3D12_BLEND_SRC1_COLOR;
		case BlendFactor::OneMinusSrc1Color:	  return D3D12_BLEND_INV_SRC1_COLOR;
		case BlendFactor::Src1Alpha:			  return D3D12_BLEND_SRC1_ALPHA;
		case BlendFactor::OneMinusSrc1Alpha:	  return D3D12_BLEND_INV_SRC1_ALPHA;
		default:
			return D3D12_BLEND_ZERO;
		}
	}
	
	 
	D3D12_BLEND_OP ToD3D12BlendOp(BlendOp op) noexcept {
		switch (op) {
		case BlendOp::Add:			  return D3D12_BLEND_OP_ADD;
		case BlendOp::Subtract:		 return D3D12_BLEND_OP_SUBTRACT;
		case BlendOp::ReverseSubtract:  return D3D12_BLEND_OP_REV_SUBTRACT;
		case BlendOp::Min:			   return D3D12_BLEND_OP_MIN;
		case BlendOp::Max:			   return D3D12_BLEND_OP_MAX;
		default:
			return D3D12_BLEND_OP_ADD;
		}
	}

	 
	D3D12_LOGIC_OP ToD3D12LogicOp(LogicOp op) noexcept {
		switch (op) {
		case LogicOp::Clear:		return D3D12_LOGIC_OP_CLEAR;
		case LogicOp::And:		  return D3D12_LOGIC_OP_AND;
		case LogicOp::AndReverse:   return D3D12_LOGIC_OP_AND_REVERSE;
		case LogicOp::Copy:		 return D3D12_LOGIC_OP_COPY;
		case LogicOp::AndInverted:  return D3D12_LOGIC_OP_AND_INVERTED;
		case LogicOp::Noop:		 return D3D12_LOGIC_OP_NOOP;
		case LogicOp::Xor:		  return D3D12_LOGIC_OP_XOR;
		case LogicOp::Or:		   return D3D12_LOGIC_OP_OR;
		case LogicOp::Nor:		  return D3D12_LOGIC_OP_NOR;
		case LogicOp::Equiv:		return D3D12_LOGIC_OP_EQUIV;
		case LogicOp::Invert:	   return D3D12_LOGIC_OP_INVERT;
		case LogicOp::OrReverse:	return D3D12_LOGIC_OP_OR_REVERSE;
		case LogicOp::CopyInverted: return D3D12_LOGIC_OP_COPY_INVERTED;
		case LogicOp::OrInverted:   return D3D12_LOGIC_OP_OR_INVERTED;
		case LogicOp::Nand:		 return D3D12_LOGIC_OP_NAND;
		case LogicOp::Set:		  return D3D12_LOGIC_OP_SET;
		default: 					return D3D12_LOGIC_OP_COPY;
		}
	}

	 
	D3D12_FILL_MODE ToD3D12FillMode(GPUExecutableFlags fill_mode) {
	
		if (HasConflictingFlags(fill_mode, GPUExecutableFlagBits::FillModeAll)) {
			throw std::invalid_argument("Multiple fill modes specified");
		}

		if (fill_mode & GPUExecutableFlagBits::FillModeSolid) {
			return D3D12_FILL_MODE_SOLID;
		}

		if (fill_mode & GPUExecutableFlagBits::FillModeWireframe) {
			return D3D12_FILL_MODE_WIREFRAME;
		}

		throw std::invalid_argument("No fill mode specified");

	}

	
	D3D12_CULL_MODE ToD3D12CullMode(GPUExecutableFlags cull_mode) {
	
		if (fyuu_rhi::HasConflictingFlags(cull_mode, GPUExecutableFlagBits::CullModeAll)) {
			throw std::invalid_argument("Multiple fill modes specified");
		}

		if (cull_mode & GPUExecutableFlagBits::CullModeNone) {
			return D3D12_CULL_MODE_NONE;
		}
		if (cull_mode & GPUExecutableFlagBits::CullModeFront) {
			return D3D12_CULL_MODE_FRONT;
		}
		if (cull_mode & GPUExecutableFlagBits::CullModeBack) {
			return D3D12_CULL_MODE_BACK;
		}
	
		throw std::invalid_argument("No cull mode specified");

	}
	
	D3D12_COMPARISON_FUNC ToD3D12ComparisonFunction(CompareOp op) noexcept {
		switch (op) {
		case CompareOp::Never:			return D3D12_COMPARISON_FUNC_NEVER;
		case CompareOp::Less:			return D3D12_COMPARISON_FUNC_LESS;
		case CompareOp::Equal:			return D3D12_COMPARISON_FUNC_EQUAL;
		case CompareOp::LessOrEqual:	return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case CompareOp::Greater:		return D3D12_COMPARISON_FUNC_GREATER;
		case CompareOp::NotEqual:		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case CompareOp::GreaterOrEqual:	return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case CompareOp::Always:			return D3D12_COMPARISON_FUNC_ALWAYS;
		default: 						return D3D12_COMPARISON_FUNC_NEVER;
		}
	}
	
	D3D12_STENCIL_OP ToD3D12StencilOp(StencilOp op) noexcept {
		switch (op) {
		case StencilOp::Keep:				return D3D12_STENCIL_OP_KEEP;
		case StencilOp::Zero:				return D3D12_STENCIL_OP_ZERO;
		case StencilOp::Replace:			return D3D12_STENCIL_OP_REPLACE;
		case StencilOp::IncrementAndClamp:	return D3D12_STENCIL_OP_INCR_SAT;
		case StencilOp::DecrementAndClamp:	return D3D12_STENCIL_OP_DECR_SAT;
		case StencilOp::Invert:				return D3D12_STENCIL_OP_INVERT;
		case StencilOp::IncrementAndWrap:	return D3D12_STENCIL_OP_INCR;
		case StencilOp::DecrementAndWrap:	return D3D12_STENCIL_OP_DECR;
		default: 							return D3D12_STENCIL_OP_KEEP;
		}
	}

	D3D12_PRIMITIVE_TOPOLOGY_TYPE ToD3D12PrimitiveTopology(GPUExecutableFlags flags) noexcept {

		if (HasConflictingFlags(flags, GPUExecutableFlagBits::PrimitiveTopologyAll)) {
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		}

		auto topo_bits = flags & GPUExecutableFlagBits::PrimitiveTopologyAll;

		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyPointList)
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyLineList || topo_bits == GPUExecutableFlagBits::PrimitiveTopologyLineStrip)
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyTriangleList || topo_bits == GPUExecutableFlagBits::PrimitiveTopologyTriangleStrip)
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyPatchList)
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;

		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	}

	D3D12_PRIMITIVE_TOPOLOGY ToD3DPrimitiveTopology(GPUExecutableFlags flags) noexcept {

		auto topo_bits = flags & GPUExecutableFlagBits::PrimitiveTopologyAll;

		if (HasConflictingFlags(flags, GPUExecutableFlagBits::PrimitiveTopologyAll)) {
			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	
		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyPointList)
			return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyLineList)
			return D3D_PRIMITIVE_TOPOLOGY_LINELIST;

		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyLineStrip)
			return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;

		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyTriangleList)
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyTriangleStrip)
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyPatchList)
			// TODO: implement this, need other information from HULL shader 
			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	

	}

	D3D12_COLOR_WRITE_ENABLE ToD3D12ColorWriteEnable(std::uint8_t color_write_mask) noexcept {
		std::uint8_t result = 0x00;
		if (color_write_mask & 0x01) result |= D3D12_COLOR_WRITE_ENABLE_RED;
		if (color_write_mask & 0x02) result |= D3D12_COLOR_WRITE_ENABLE_GREEN;
		if (color_write_mask & 0x04) result |= D3D12_COLOR_WRITE_ENABLE_BLUE;
		if (color_write_mask & 0x08) result |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
		return static_cast<D3D12_COLOR_WRITE_ENABLE>(result);
	}

}

namespace fyuu_rhi::d3d12 {

#define VALIDATE_LOGICAL_DEVICE \
		if (!m_logical_device_id) {	\
			throw std::invalid_argument("invalid logical device");	\
		}	\
		auto* logical_device = plastic::utility::QueryObject<D3D12LogicalDevice>(*m_logical_device_id);	\
		if (!logical_device) {	\
			throw std::runtime_error("logical device lost");	\
		}

	void D3D12GPUExecutable::CompileGraphicsExecutable(Declaration* declaration) {

		VALIDATE_LOGICAL_DEVICE

		auto& shader_data_segment_id = declaration->shader_data_segment_id;
		if (!shader_data_segment_id) {
			throw std::invalid_argument("invalid shader data segment");
		}
		auto* shader_data_segment = plastic::utility::QueryObject<D3D12ShaderDataSegment>(*shader_data_segment_id);
		if (!shader_data_segment) {
			throw std::runtime_error("shader data segment lost");
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};

		Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature = shader_data_segment->GetInstantiatedNative();
		desc.pRootSignature = root_signature.Get();

		SmallVector<Microsoft::WRL::ComPtr<IDxcBlob>, 8u> shaders;

		for (auto const& [stage, shader_id] : declaration->shaders) {

			if (!shader_id) {
				throw std::invalid_argument("invalid shader");
			}
			auto* shader = plastic::utility::QueryObject<D3D12Shader>(*shader_id);
			if (!shader) {
				throw std::runtime_error("shader lost");
			}	

			D3D12_SHADER_BYTECODE* bc = nullptr;
			switch (stage) {
			case ShaderStage::Vertex:
				bc = &desc.VS;
				break;

			case ShaderStage::Pixel:
				bc = &desc.PS;
				break;

			case ShaderStage::Geometry:
				bc = &desc.GS;
				break;

			case ShaderStage::Hull:
				bc = &desc.HS;
				break;

			case ShaderStage::Domain:
				bc = &desc.DS;
				break;

			default:
				continue;
			}

			Microsoft::WRL::ComPtr<IDxcBlob>& blob = shaders.emplace_back(shader->GetNative());
			bc->pShaderBytecode = blob->GetBufferPointer();
			bc->BytecodeLength = blob->GetBufferSize();

		}

		auto& rt_att = declaration->rt_att;
		D3D12_BLEND_DESC& blend = desc.BlendState; 

		std::visit(
			[&rt_att, &blend](auto&& blend_state) {
				using BlendState = std::decay_t<decltype(blend_state)>;
				if constexpr (std::same_as<BlendState, std::vector<BlendInfo>>) {
					auto iter = std::adjacent_find(blend_state.begin(), blend_state.end());
					blend.IndependentBlendEnable = iter != blend_state.end();
					std::size_t length = blend.IndependentBlendEnable ? (std::min)({ blend_state.size(), std::size_t(D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT), rt_att.color_write_masks.size() }) : 1u;
					for (std::size_t i = 0; i < length; ++i) {
						
						auto& rt = blend.RenderTarget[i];
						auto& color_write_mask = rt_att.color_write_masks[i];
						auto& blend_info = blend_state[i];

						rt.BlendEnable = true;
						rt.LogicOpEnable = false;
						rt.SrcBlend = ToD3D12BlendFactor(blend_info.src_color_blend_factor);
						rt.DestBlend = ToD3D12BlendFactor(blend_info.dst_color_blend_factor);
						rt.BlendOp = ToD3D12BlendOp(blend_info.color_blend_op);
						rt.SrcBlendAlpha = ToD3D12BlendFactor(blend_info.src_alpha_blend_factor);
						rt.DestBlendAlpha = ToD3D12BlendFactor(blend_info.dst_alpha_blend_factor);
						rt.BlendOpAlpha = ToD3D12BlendOp(blend_info.alpha_blend_op);
						rt.RenderTargetWriteMask = ToD3D12ColorWriteEnable(color_write_mask);

					}
				}
				else if constexpr (std::same_as<BlendState, LogicOp>) {
					blend.IndependentBlendEnable = false;
					auto& rt = blend.RenderTarget[0];
					rt.BlendEnable = false;
					rt.LogicOpEnable = true;
					rt.LogicOp = ToD3D12LogicOp(blend_state);
					rt.RenderTargetWriteMask = ToD3D12ColorWriteEnable(rt_att.color_write_masks[0]);
				}
				else {

				}
			},
			rt_att.blend_state
		);
		
		auto& flags = declaration->flags;

		D3D12_RASTERIZER_DESC& rasterizer = desc.RasterizerState;
		rasterizer.FillMode = ToD3D12FillMode(flags);
		rasterizer.CullMode = ToD3D12CullMode(flags);
		rasterizer.FrontCounterClockwise = flags & GPUExecutableFlagBits::FrontCounterClockwise;
		if (declaration->depth_bias) {
			auto& depth_bias = *declaration->depth_bias;
			rasterizer.DepthBias = static_cast<INT>(depth_bias.constant_factor);
			rasterizer.DepthBiasClamp = depth_bias.clamp;
			rasterizer.SlopeScaledDepthBias = depth_bias.slope_factor;
		}
		rasterizer.DepthClipEnable = flags & GPUExecutableFlagBits::EnableDepthClip;
		rasterizer.AntialiasedLineEnable = false;
		rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		D3D12_DEPTH_STENCIL_DESC& depth_stencil = desc.DepthStencilState;
		if (declaration->depth_stencil) {
			auto& ds = declaration->depth_stencil;
			auto& depth = ds->depth;
			auto& stencil = ds->stencil;

			depth_stencil.DepthEnable = depth.has_value();
			if (depth_stencil.DepthEnable) {
				depth_stencil.DepthWriteMask = depth->enable_write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
				depth_stencil.DepthFunc = ToD3D12ComparisonFunction(depth->compare);
			}
			
			depth_stencil.StencilEnable = stencil.has_value();
			if (depth_stencil.StencilEnable) {

				depth_stencil.StencilReadMask = stencil->read_mask;
				depth_stencil.StencilWriteMask = stencil->write_mask;

				depth_stencil.FrontFace.StencilFailOp = ToD3D12StencilOp(stencil->front.failed);
				depth_stencil.FrontFace.StencilDepthFailOp = ToD3D12StencilOp(stencil->front.depth_failed);
				depth_stencil.FrontFace.StencilPassOp = ToD3D12StencilOp(stencil->front.pass);
				depth_stencil.FrontFace.StencilFunc = ToD3D12ComparisonFunction(stencil->front.compare);

				depth_stencil.BackFace.StencilFailOp = ToD3D12StencilOp(stencil->back.failed);
				depth_stencil.BackFace.StencilDepthFailOp = ToD3D12StencilOp(stencil->back.depth_failed);
				depth_stencil.BackFace.StencilPassOp = ToD3D12StencilOp(stencil->back.pass);
				depth_stencil.BackFace.StencilFunc = ToD3D12ComparisonFunction(stencil->back.compare);

			}

			desc.DSVFormat = DetermineFormat(ds->format);

		}

		D3D12_INPUT_LAYOUT_DESC& input_layout = desc.InputLayout;
		auto& vertex_input_layout = declaration->vertex_input_layout;
		if (vertex_input_layout.bindings.empty()) {
			throw std::invalid_argument("Invalid vertex binding");
		}

		if (vertex_input_layout.attributes.empty()) {
			throw std::invalid_argument("Invalid vertex attributes");
		}

		std::unordered_map<std::size_t, VertexBinding const*> binding_map;
		for (auto const& b : vertex_input_layout.bindings) {
			auto [it, inserted] = binding_map.emplace(b.where, &b);
			if (!inserted) {
				auto const& existing = *it->second;
				if (existing.stride != b.stride || existing.input_rate != b.input_rate) {
					throw std::invalid_argument(std::format("Inconsistent stride or input rate for binding {}", b.where));
				}
			}
		}

		for (auto const& attr : vertex_input_layout.attributes) {
			if (!binding_map.contains(attr.where)) {
				throw std::invalid_argument(std::format("Attribute references missing binding {}", attr.where));
			}
		}

		std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
		elements.reserve(vertex_input_layout.attributes.size());

		for (auto const& attribute : vertex_input_layout.attributes) {

			auto const& binding = *binding_map[attribute.where];
			D3D12_INPUT_ELEMENT_DESC& element = elements.emplace_back();

			element.InputSlot = static_cast<UINT>(binding.where);
			element.AlignedByteOffset = static_cast<UINT>(attribute.offset);
			switch (binding.input_rate) {
			case InputRate::Vertex:
				element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
				element.InstanceDataStepRate = 0;
				break;

			case InputRate::Instance:
				element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
				element.InstanceDataStepRate = 1u;
				break;

			default:
				throw std::invalid_argument("Invalid input rate");
			}

			std::visit(
				[&element](auto&& id) {
					using ID = std::decay_t<decltype(id)>;
					if constexpr (std::same_as<ID, SemanticInfo>) {
						element.SemanticName = id.name.data();
						element.SemanticIndex = id.index;
					}
					else if constexpr (std::same_as<ID, std::uint32_t>) {
						element.SemanticName = "TEXCOORD";
						element.SemanticIndex = id;
					}
					else {
						throw std::invalid_argument("Invalid vertex attribute identifier");
					}
				},
				attribute.id
			);

			element.Format = DetermineFormat(attribute.format);

		}
		input_layout.pInputElementDescs = elements.data();
		input_layout.NumElements = static_cast<UINT>(elements.size());

		auto& rt_formats = declaration->rt_formats;
		if (rt_formats.empty()) {
			throw std::invalid_argument("Render target format not set");
		}
		
		std::span<DXGI_FORMAT> formats_span(desc.RTVFormats);
		auto result = std::ranges::transform(
			rt_formats | std::ranges::views::take(formats_span.size()),
			formats_span.begin(),
			[](ResourceFlags format) { 
				return DetermineFormat(format); 
			}
		);
		std::ranges::fill(result.out, formats_span.end(), DXGI_FORMAT_UNKNOWN);
		desc.NumRenderTargets = result.out - formats_span.begin();

		DXGI_SAMPLE_DESC& sampler = desc.SampleDesc;
		auto& ms = declaration->multi_sample;
		if (ms) {

			SmallVector<std::size_t, 8u> qualities;
			std::ranges::transform(
				formats_span,
				std::back_inserter(qualities),
				[logical_device, &ms](DXGI_FORMAT f) {
					if (f == DXGI_FORMAT_UNKNOWN) {
						return 0u;
					}
					D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ms_quality_levels{
						f,
						ms->count,
						D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,
						0u
					};

					HRESULT hr = logical_device->GetRawNative()->CheckFeatureSupport(
						D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
						&ms_quality_levels,
						sizeof(ms_quality_levels)
					);

					if (SUCCEEDED(hr) && ms_quality_levels.NumQualityLevels > 0u) {
						UINT supported_quality_count = ms_quality_levels.NumQualityLevels;
						return supported_quality_count - 1u; 
					} 
					else {
						return 0u;
					}
				}
			);

			std::size_t quality;
			auto iter = std::adjacent_find(
				formats_span.begin(), 
				formats_span.begin() + desc.NumRenderTargets, 
				[&quality](std::size_t qa, std::size_t qb) {
					quality = qa;
					return qa != qb;
				}
			);
			if (iter != formats_span.end()) {
				throw std::invalid_argument("sample quality does not match");
			}

			rasterizer.MultisampleEnable = true;
			sampler.Count = ms->count;
			sampler.Quality = quality;
			desc.SampleMask = ms->mask;
			blend.AlphaToCoverageEnable = ms->enable_alpha_to_coverage;

		}
		else {
			desc.SampleMask = (std::numeric_limits<UINT>::max)();
			sampler.Count = 1u;
			sampler.Quality = 0u;
		}

		desc.PrimitiveTopologyType = ToD3D12PrimitiveTopology(flags);
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso16;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso32;
		if (flags & GPUExecutableFlagBits::EnablePrimitiveRestart) {
			desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
			pso16 = logical_device->CreatePipeline(desc);
			desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
			pso32 = logical_device->CreatePipeline(desc);
		}
		else {
			pso16 = logical_device->CreatePipeline(desc);
			pso32 = pso16;
		}
		
		m_handle.emplace<Compilation>(
			shader_data_segment_id,
			std::move(root_signature),
			ToD3DPrimitiveTopology(flags),
			GraphicsPSO(pso16, pso32)
		);

	}

	void D3D12GPUExecutable::CompileComputingExecutable(Declaration* declaration) {

		VALIDATE_LOGICAL_DEVICE

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};

		auto& shader_data_segment_id = declaration->shader_data_segment_id;
		if (!shader_data_segment_id) {
			throw std::invalid_argument("invalid shader data segment");
		}
		auto* shader_data_segment = plastic::utility::QueryObject<D3D12ShaderDataSegment>(*shader_data_segment_id);
		if (!shader_data_segment) {
			throw std::runtime_error("shader data segment lost");
		}

		Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature = shader_data_segment->GetInstantiatedNative();
		desc.pRootSignature = root_signature.Get();

		SmallVector<Microsoft::WRL::ComPtr<IDxcBlob>, 1u> shaders;

		for (auto const& [stage, shader_id] : declaration->shaders) {

			if (!shader_id) {
				throw std::invalid_argument("invalid shader");
			}
			auto* shader = plastic::utility::QueryObject<D3D12Shader>(*shader_id);
			if (!shader) {
				throw std::runtime_error("shader lost");
			}	

			D3D12_SHADER_BYTECODE* bc = nullptr;
			switch (stage) {
			case ShaderStage::Compute:
				bc = &desc.CS;
				break;

			default:
				continue;
			}

			Microsoft::WRL::ComPtr<IDxcBlob>& blob = shaders.emplace_back(shader->GetNative());
			bc->pShaderBytecode = blob->GetBufferPointer();
			bc->BytecodeLength = blob->GetBufferSize();

		}
		
		m_handle.emplace<Compilation>(
			shader_data_segment_id,
			std::move(root_signature),
			D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,
			ComputePSO(logical_device->CreatePipeline(desc))
		);

	}

	void D3D12GPUExecutable::CompileMeshExecutable(Declaration* declaration)
	{
	}

	void D3D12GPUExecutable::CompileRayTracingExecutable(Declaration* declaration)
	{
	}

	D3D12GPUExecutable::D3D12GPUExecutable(D3D12LogicalDevice const& logical_device)
		: PolymorphicGPUExecutableBase(this),
		D3D12Common(this),
		m_logical_device_id(logical_device.GetID()),
		m_handle() {
	}

	D3D12GPUExecutable& D3D12GPUExecutable::SetShader(ShaderStage stage, D3D12Shader const& shader) {
		
		std::visit(
			[stage, &shader](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, D3D12GPUExecutable::Declaration>) {
					if (HasConflictingFlags(stage, ShaderStage::All)) {
						throw std::invalid_argument("Invalid stage: Specifies multiple shader stages");
					}
					handle.shaders[stage] = shader.GetID();
				}
				else {
					throw std::logic_error("Cannot set after compilation");
				}
			},
			m_handle
		);

		return *this;

	}

#define DECLARE_SINGLE(SET_STATE)	\
	std::visit(	\
		[&](auto&& handle) {	\
			using Handle = std::decay_t<decltype(handle)>;	\
			if constexpr (std::same_as<Handle, D3D12GPUExecutable::Declaration>) {	\
				SET_STATE;	\
			}	\
			else {	\
				throw std::logic_error("Cannot set after compilation");	\
			}	\
		},	\
		m_handle	\
	);	\
	\
	return *this;

	D3D12GPUExecutable& D3D12GPUExecutable::SetShaderDataSegment(D3D12ShaderDataSegment const& shader_data_segment) {
		DECLARE_SINGLE(handle.shader_data_segment_id = shader_data_segment.GetID())
	}

	D3D12GPUExecutable& D3D12GPUExecutable::SetFlags(GPUExecutableFlags const& flags) {
		DECLARE_SINGLE(handle.flags = flags)
	}

	D3D12GPUExecutable& D3D12GPUExecutable::SetRenderTargetAttachments(RenderTargetAttachmentsInfo const& info) {
		DECLARE_SINGLE(handle.rt_att = info)
	}

	D3D12GPUExecutable& D3D12GPUExecutable::SetRenderTargetAttachments(std::span<BlendInfo const> blend_info, std::span<std::uint8_t const> color_write_masks) {
		
		std::visit(
			[blend_info, color_write_masks](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, D3D12GPUExecutable::Declaration>) {
					auto& rt_att = handle.rt_att;
					rt_att.blend_state.template emplace<std::vector<BlendInfo>>(std::from_range, blend_info);
					rt_att.color_write_masks.assign(color_write_masks.begin(), color_write_masks.end());
				}
				else {
					throw std::logic_error("Cannot set after compilation");
				}
			},
			m_handle
		);

		return *this;

	}

	D3D12GPUExecutable& D3D12GPUExecutable::SetRenderTargetAttachments(LogicOp logic_op, std::span<std::uint8_t const> color_write_masks) {
		
		std::visit(
			[logic_op, color_write_masks](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, D3D12GPUExecutable::Declaration>) {
					auto& rt_att = handle.rt_att;
					rt_att.blend_state.template emplace<LogicOp>(logic_op);
					rt_att.color_write_masks.assign(color_write_masks.begin(), color_write_masks.end());
				}
				else {
					throw std::logic_error("Cannot set after compilation");
				}
			},
			m_handle
		);

		return *this;

	}

	D3D12GPUExecutable& D3D12GPUExecutable::SetDepthBias(DepthBias const &depth_bias){
		DECLARE_SINGLE(handle.depth_bias.emplace(depth_bias))}

	D3D12GPUExecutable& D3D12GPUExecutable::SetDepthStencil(DepthStencilInfo const& depth_stencil){
		DECLARE_SINGLE(handle.depth_stencil.emplace(depth_stencil))}

	D3D12GPUExecutable& D3D12GPUExecutable::SetMultiSample(MultiSample const& multi_sample){
		DECLARE_SINGLE(handle.multi_sample.emplace(multi_sample))}

	D3D12GPUExecutable& D3D12GPUExecutable::SetRenderTargetFormats(std::span<ResourceFlags const> format) {
		DECLARE_SINGLE(handle.rt_formats.assign(format.begin(), format.end()))
	}

	D3D12GPUExecutable& D3D12GPUExecutable::SetVertexInputLayout(VertexInputLayout const& vertex_input_layout) {
		DECLARE_SINGLE(handle.vertex_input_layout = vertex_input_layout)
	}

	D3D12GPUExecutable& D3D12GPUExecutable::SetVertexInputLayout(std::span<VertexBinding const> bindings, std::span<VertexAttribute const> attributes) {
		std::visit(
			[bindings, attributes](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, D3D12GPUExecutable::Declaration>) {
					auto& vertex_input_layout = handle.vertex_input_layout;
					vertex_input_layout.bindings.assign(bindings.begin(), bindings.end());
					vertex_input_layout.attributes.assign(attributes.begin(), attributes.end());
				}
				else {
					throw std::logic_error("Cannot set after compilation");
				}
			},
			m_handle
		);

		return *this;
	}

	D3D12GPUExecutable& D3D12GPUExecutable::AddVertexInputLayout(VertexBinding const& binding, VertexAttribute const& attribute) {
		std::visit(
			[&binding, &attribute](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, D3D12GPUExecutable::Declaration>) {
					auto& vertex_input_layout = handle.vertex_input_layout;
					vertex_input_layout.bindings.emplace_back(binding);
					vertex_input_layout.attributes.emplace_back(attribute);
				}
				else {
					throw std::logic_error("Cannot set after compilation");
				}
			},
			m_handle
		);

		return *this;
	}

	D3D12GPUExecutable& D3D12GPUExecutable::AddVertexBinding(VertexBinding const& binding) {
		std::visit(
			[&binding](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, D3D12GPUExecutable::Declaration>) {
					auto& vertex_input_layout = handle.vertex_input_layout;
					vertex_input_layout.bindings.emplace_back(binding);
				}
				else {
					throw std::logic_error("Cannot set after compilation");
				}
			},
			m_handle
		);

		return *this;
	}

	D3D12GPUExecutable& D3D12GPUExecutable::AddVertexAttribute(VertexAttribute const& attribute){
		std::visit(
			[&attribute](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, D3D12GPUExecutable::Declaration>) {
					auto& vertex_input_layout = handle.vertex_input_layout;
					vertex_input_layout.attributes.emplace_back(attribute);
				}
				else {
					throw std::logic_error("Cannot set after compilation");
				}
			},
			m_handle
		);

		return *this;
	}

	void D3D12GPUExecutable::Compile() {
	
		auto* declaration = std::get_if<Declaration>(&m_handle);
		if (!declaration) {
			throw std::logic_error("Already compiled");
		}
		auto& flags = declaration->flags;

		if (HasConflictingFlags(flags, GPUExecutableFlagBits::AllType)) {
			throw std::invalid_argument("Invalid parameters: Multiple types were set.");
		}

		if (flags & GPUExecutableFlagBits::Graphics) {
			CompileGraphicsExecutable(declaration);
			return;
		}
		
		if (flags & GPUExecutableFlagBits::Compute) {
			CompileComputingExecutable(declaration);
			return;
		}

		if (flags & GPUExecutableFlagBits::Mesh) {
			CompileMeshExecutable(declaration);
			return;
		}

		if (flags & GPUExecutableFlagBits::RayTracing) {
			CompileRayTracingExecutable(declaration);
			return;
		}

		throw std::invalid_argument("Invalid parameter: No type set.");

	}

	void D3D12GPUExecutable::Reset() {
		m_handle.emplace<Declaration>();
	}

#define ENSURE_COMPILATION	\
	auto* compilation = std::get_if<Compilation>(&m_handle);	\
	if (!compilation) return {};

	std::optional<std::size_t> D3D12GPUExecutable::GetAssociatedShaderDataSegmentID() const noexcept {
		ENSURE_COMPILATION
		return compilation->shader_data_segment_id;
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> D3D12GPUExecutable::GetAssociatedRootSignature() const noexcept {
		ENSURE_COMPILATION
		return compilation->root_signature;
	}

	ID3D12RootSignature* D3D12GPUExecutable::GetRawAssociatedRootSignature() const noexcept {
		ENSURE_COMPILATION
		return compilation->root_signature.Get();
	}

	D3D12_PRIMITIVE_TOPOLOGY D3D12GPUExecutable::GetPrimitiveTopology() const noexcept {
		ENSURE_COMPILATION
		return compilation->primitive_topology;
	}

} // namespace fyuu_rhi::d3d12

#endif // defined(_WIN32)