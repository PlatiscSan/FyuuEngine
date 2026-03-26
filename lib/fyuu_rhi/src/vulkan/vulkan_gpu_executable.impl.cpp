/* vulkan_gpu_executable.impl.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <algorithm>
#include <array>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>
#include <span>
#include <format>
#include <ranges>
#endif // !defined(__cpp_lib_modules)
#include "declare_pool.hpp"
module fyuu_rhi:vulkan_gpu_executable_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :vulkan_gpu_executable;
import vulkan;
import :types;
import :vulkan_common;
import :vulkan_shader_data_segment;
import :vulkan_shader;
import :vulkan_logical_device;
import :vulkan_types;

namespace {

	using namespace fyuu_rhi;
	using namespace fyuu_rhi::vulkan;

	vk::BlendFactor ToVulkanBlendFactor(BlendFactor factor) noexcept {
		switch (factor) {
		case BlendFactor::Zero:				  return vk::BlendFactor::eZero;
		case BlendFactor::One:				   return vk::BlendFactor::eOne;
		case BlendFactor::SrcColor:			  return vk::BlendFactor::eSrcColor;
		case BlendFactor::OneMinusSrcColor:	  return vk::BlendFactor::eOneMinusSrcColor;
		case BlendFactor::DstColor:			  return vk::BlendFactor::eDstColor;
		case BlendFactor::OneMinusDstColor:	  return vk::BlendFactor::eOneMinusDstColor;
		case BlendFactor::SrcAlpha:			   return vk::BlendFactor::eSrcAlpha;
		case BlendFactor::OneMinusSrcAlpha:	   return vk::BlendFactor::eOneMinusSrcAlpha;
		case BlendFactor::DstAlpha:			   return vk::BlendFactor::eDstAlpha;
		case BlendFactor::OneMinusDstAlpha:	   return vk::BlendFactor::eOneMinusDstAlpha;
		case BlendFactor::ConstantColor:		  return vk::BlendFactor::eConstantColor;
		case BlendFactor::OneMinusConstantColor:  return vk::BlendFactor::eOneMinusConstantColor;
		case BlendFactor::ConstantAlpha:		  return vk::BlendFactor::eConstantAlpha;
		case BlendFactor::OneMinusConstantAlpha:  return vk::BlendFactor::eOneMinusConstantAlpha;
		case BlendFactor::SrcAlphaSaturate:	   return vk::BlendFactor::eSrcAlphaSaturate;
		case BlendFactor::Src1Color:			  return vk::BlendFactor::eSrc1Color;
		case BlendFactor::OneMinusSrc1Color:	  return vk::BlendFactor::eOneMinusSrc1Color;
		case BlendFactor::Src1Alpha:			  return vk::BlendFactor::eSrc1Alpha;
		case BlendFactor::OneMinusSrc1Alpha:	  return vk::BlendFactor::eOneMinusSrc1Alpha;
		default:
			return vk::BlendFactor::eZero;
		}
	}
	
	vk::BlendOp ToVulkanBlendOp(BlendOp op) noexcept {
		switch (op) {
		case BlendOp::Add:			  return vk::BlendOp::eAdd;
		case BlendOp::Subtract:		 return vk::BlendOp::eSubtract;
		case BlendOp::ReverseSubtract:  return vk::BlendOp::eReverseSubtract;
		case BlendOp::Min:			   return vk::BlendOp::eMin;
		case BlendOp::Max:			   return vk::BlendOp::eMax;
		default:
			return vk::BlendOp::eAdd;
		}
	}

	vk::ColorComponentFlags ToVulkanColorComponent(std::uint8_t color_write_mask) noexcept {
		vk::ColorComponentFlags result;
		if (color_write_mask & 0x01) result |= vk::ColorComponentFlagBits::eR;
		if (color_write_mask & 0x02) result |= vk::ColorComponentFlagBits::eG;
		if (color_write_mask & 0x04) result |= vk::ColorComponentFlagBits::eB;
		if (color_write_mask & 0x08) result |= vk::ColorComponentFlagBits::eA;
		return result;
	}

	vk::LogicOp ToVulkanLogicOp(LogicOp op) noexcept {
		switch (op) {
		case LogicOp::Clear:		return vk::LogicOp::eClear;
		case LogicOp::And:		  return vk::LogicOp::eAnd;
		case LogicOp::AndReverse:   return vk::LogicOp::eAndReverse;
		case LogicOp::Copy:		 return vk::LogicOp::eCopy;
		case LogicOp::AndInverted:  return vk::LogicOp::eAndInverted;
		case LogicOp::Noop:		 return vk::LogicOp::eNoOp;
		case LogicOp::Xor:		  return vk::LogicOp::eXor;
		case LogicOp::Or:		   return vk::LogicOp::eOr;
		case LogicOp::Nor:		  return vk::LogicOp::eNor;
		case LogicOp::Equiv:		return vk::LogicOp::eEquivalent;
		case LogicOp::Invert:	   return vk::LogicOp::eInvert;
		case LogicOp::OrReverse:	return vk::LogicOp::eOrReverse;
		case LogicOp::CopyInverted: return vk::LogicOp::eCopyInverted;
		case LogicOp::OrInverted:   return vk::LogicOp::eOrInverted;
		case LogicOp::Nand:		 return vk::LogicOp::eNand;
		case LogicOp::Set:		  return vk::LogicOp::eSet;
		default:
			return vk::LogicOp::eCopy; // fallback
		}
	}

	vk::PolygonMode ToVulkanFillMode(GPUExecutableFlags fill_mode) {

		if (HasConflictingFlags(fill_mode, GPUExecutableFlagBits::FillModeAll)) {
			throw std::invalid_argument("Multiple fill modes specified");
		}
	
		if (fill_mode & GPUExecutableFlagBits::FillModeSolid) {
			return vk::PolygonMode::eFill;
		}

		if (fill_mode & GPUExecutableFlagBits::FillModeWireframe) {
			return vk::PolygonMode::eLine;
		}

		return vk::PolygonMode::eFill;
	}

	vk::CullModeFlags ToVulkanCullMode(GPUExecutableFlags cull_mode) {
		
		if (HasConflictingFlags(cull_mode, GPUExecutableFlagBits::CullModeAll)) {
			throw std::invalid_argument("Multiple fill modes specified");
		}

		if (cull_mode & GPUExecutableFlagBits::CullModeNone) {
			return vk::CullModeFlagBits::eNone;
		}
		if (cull_mode & GPUExecutableFlagBits::CullModeFront) {
			return vk::CullModeFlagBits::eFront;
		}
		if (cull_mode & GPUExecutableFlagBits::CullModeBack) {
			return vk::CullModeFlagBits::eBack;
		}
	
		throw std::invalid_argument("No cull mode specified");
	}
		
	vk::CompareOp ToVulkanCompareOp(CompareOp op) noexcept {
		switch (op) {
		case CompareOp::Never:			return vk::CompareOp::eNever;
		case CompareOp::Less:			return vk::CompareOp::eLess;
		case CompareOp::Equal:			return vk::CompareOp::eEqual;
		case CompareOp::LessOrEqual:	return vk::CompareOp::eLessOrEqual;
		case CompareOp::Greater:		return vk::CompareOp::eGreater;
		case CompareOp::NotEqual:		return vk::CompareOp::eNotEqual;
		case CompareOp::GreaterOrEqual:	return vk::CompareOp::eGreaterOrEqual;
		case CompareOp::Always:			return vk::CompareOp::eAlways;
		default:						return vk::CompareOp::eNever;	
		}
	}

	vk::StencilOp ToVulkanStencilOp(StencilOp op) noexcept {
		switch (op) {
		case StencilOp::Keep:				return vk::StencilOp::eKeep;
		case StencilOp::Zero:				return vk::StencilOp::eZero;
		case StencilOp::Replace:			return vk::StencilOp::eReplace;
		case StencilOp::IncrementAndClamp:	return vk::StencilOp::eIncrementAndClamp;
		case StencilOp::DecrementAndClamp:	return vk::StencilOp::eDecrementAndClamp;
		case StencilOp::Invert:				return vk::StencilOp::eInvert;
		case StencilOp::IncrementAndWrap:	return vk::StencilOp::eIncrementAndWrap;
		case StencilOp::DecrementAndWrap:	return vk::StencilOp::eDecrementAndWrap;
		default: 							return vk::StencilOp::eKeep;
		}	

	}

	vk::SampleCountFlagBits SampleCountToFlag(std::size_t count) {
		if (count == 0 || (count & (count - 1)) != 0 || count > 64) {
			throw std::invalid_argument("Invalid sample count");
		}
		unsigned int bit_pos = static_cast<unsigned int>(std::countr_zero(count));
		return static_cast<vk::SampleCountFlagBits>(1u << bit_pos);
	}

	vk::PrimitiveTopology ToVulkanPrimitiveTopology(GPUExecutableFlags flags) noexcept {

		if (HasConflictingFlags(flags, GPUExecutableFlagBits::PrimitiveTopologyAll)) {
			return vk::PrimitiveTopology::ePointList;
		}

		auto topo_bits = flags & GPUExecutableFlagBits::PrimitiveTopologyAll;

		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyPointList)
			return vk::PrimitiveTopology::ePointList;

		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyLineList)
			return vk::PrimitiveTopology::eLineList;

		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyLineStrip)
			return vk::PrimitiveTopology::eLineStrip;

		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyTriangleList)
			return vk::PrimitiveTopology::eTriangleList;

		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyTriangleStrip)
			return vk::PrimitiveTopology::eTriangleStrip;

		if (topo_bits == GPUExecutableFlagBits::PrimitiveTopologyPatchList)
			return vk::PrimitiveTopology::ePatchList;

		return vk::PrimitiveTopology::ePointList;
	}
	
}

namespace fyuu_rhi::vulkan {
#define VALIDATE_LOGICAL_DEVICE \
		if (!m_logical_device_id) {	\
			throw std::invalid_argument("invalid logical device");	\
		}	\
		auto* logical_device = plastic::utility::QueryObject<VulkanLogicalDevice>(*m_logical_device_id);	\
		if (!logical_device) {	\
			throw std::runtime_error("logical device lost");	\
		}
	
	void VulkanGPUExecutable::CompileGraphicsExecutable(Declaration* declaration) {

		VALIDATE_LOGICAL_DEVICE
		auto& shader_data_segment_id = declaration->shader_data_segment_id;
		if (!shader_data_segment_id) {
			throw std::invalid_argument("invalid shader data segment");
		}
		auto* shader_data_segment = plastic::utility::QueryObject<VulkanShaderDataSegment>(*shader_data_segment_id);
		if (!shader_data_segment) {
			throw std::runtime_error("shader data segment lost");
		}	

		vk::PipelineLayout pipeline_layout = shader_data_segment->GetPipelineLayout();

		SmallVector<vk::PipelineShaderStageCreateInfo, 16u> shaders;

		for (auto const& [stage, shader_id] : declaration->shaders) {

			if (!shader_id) {
				throw std::invalid_argument("invalid shader");
			}
			auto* shader = plastic::utility::QueryObject<VulkanShader>(*shader_id);
			if (!shader) {
				throw std::runtime_error("shader lost");
			}	
			
			shaders.emplace_back(
				vk::PipelineShaderStageCreateFlags{},
				ShaderStageToVkBits(stage),
				shader->GetNative(),
				"main",
				nullptr,
				nullptr
			);

		}

		SmallVector<vk::PipelineColorBlendAttachmentState, 16u> render_target_attachments;

		auto& rt_att = declaration->rt_att;
		vk::Bool32 enable_logic_op = vk::False;
		vk::LogicOp logic_op = vk::LogicOp::eClear;

		std::visit(
			[&rt_att, &render_target_attachments, &enable_logic_op, &logic_op](auto&& blend_state) {
				using BlendState = std::decay_t<decltype(blend_state)>;
				auto SetNoBlend = [&]() {
					std::size_t length = rt_att.color_write_masks.size();
					for (std::size_t i = 0; i < length; ++i) {
						auto& color_write_mask = rt_att.color_write_masks[i];
						render_target_attachments.emplace_back(
							vk::False, 
							vk::BlendFactor::eZero,
							vk::BlendFactor::eZero,
							vk::BlendOp::eAdd,
							vk::BlendFactor::eZero,
							vk::BlendFactor::eZero,
							vk::BlendOp::eAdd,
							ToVulkanColorComponent(color_write_mask)
						);					
					}
					};

				if constexpr (std::same_as<BlendState, std::vector<BlendInfo>>) {
					std::size_t length = (std::min)({ blend_state.size(), rt_att.color_write_masks.size() });
					for (std::size_t i = 0; i < length; ++i) {
						auto& blend_info = blend_state[i];
						auto& color_write_mask = rt_att.color_write_masks[i];
						render_target_attachments.emplace_back(
							vk::True, 
							ToVulkanBlendFactor(blend_info.src_color_blend_factor),
							ToVulkanBlendFactor(blend_info.dst_color_blend_factor),
							ToVulkanBlendOp(blend_info.color_blend_op),
							ToVulkanBlendFactor(blend_info.src_alpha_blend_factor),
							ToVulkanBlendFactor(blend_info.dst_alpha_blend_factor),
							ToVulkanBlendOp(blend_info.alpha_blend_op),
							ToVulkanColorComponent(color_write_mask)
						);					
					}
				}
				else if constexpr (std::same_as<BlendState, LogicOp>) {
					SetNoBlend();
					enable_logic_op = vk::True;
					logic_op = ToVulkanLogicOp(blend_state);
				}
				else {
					SetNoBlend();
				}
			},
			rt_att.blend_state
		);

		static constexpr std::array blend_constants_placeholder = { 1.0f, 1.0f, 1.0f, 1.0f };

		vk::PipelineColorBlendStateCreateInfo blend_info(
			{},
			enable_logic_op,
			logic_op,
			render_target_attachments,
			blend_constants_placeholder,
			nullptr
		);

		auto& flags = declaration->flags;

		vk::PipelineRasterizationDepthClipStateCreateInfoEXT clip_info(
			{},
			flags & GPUExecutableFlagBits::EnableDepthClip ? vk::True : vk::False,
			nullptr
		);

		bool enable_depth_bias = declaration->depth_bias.has_value();
		float constant_factor = enable_depth_bias ? declaration->depth_bias->constant_factor : 0.0f;
		float slope_factor = enable_depth_bias ? declaration->depth_bias->slope_factor : 0.0f;
		float clamp = enable_depth_bias ? declaration->depth_bias->clamp : 0.0f;

		vk::PipelineRasterizationStateCreateInfo rasterizer_info(
			{},
			flags & GPUExecutableFlagBits::EnableDepthClip ? vk::False : vk::True,
			vk::False,
			ToVulkanFillMode(flags),
			ToVulkanCullMode(flags),
			flags & GPUExecutableFlagBits::FrontCounterClockwise ? vk::FrontFace::eCounterClockwise : vk::FrontFace::eClockwise,
			enable_depth_bias,
			constant_factor,
			slope_factor,
			clamp,
			1.0f,
			&clip_info
		);

		auto& ds = declaration->depth_stencil;

		vk::Bool32 enable_depth = vk::False;
		vk::Bool32 enable_depth_write = vk::False;
		vk::CompareOp depth_compare = vk::CompareOp::eNever;
		vk::Bool32 enable_stencil = vk::False;

		vk::StencilOp front_failed = vk::StencilOp::eKeep;
		vk::StencilOp front_pass = vk::StencilOp::eKeep;
		vk::StencilOp front_depth_failed = vk::StencilOp::eKeep;
		vk::CompareOp front_compare = vk::CompareOp::eNever;

		vk::StencilOp back_failed = vk::StencilOp::eKeep;
		vk::StencilOp back_pass = vk::StencilOp::eKeep;
		vk::StencilOp back_depth_failed = vk::StencilOp::eKeep;
		vk::CompareOp back_compare = vk::CompareOp::eNever;

		std::uint32_t stencil_read_mask = 0u;
		std::uint32_t stencil_write_mask = 0u;

		if (ds && ds->depth) {
			auto& depth = ds->depth;
			enable_depth_write = depth->enable_write ? vk::True : vk::False;
			depth_compare = ToVulkanCompareOp(depth->compare);
		}

		if (ds && ds->stencil) {
			auto& stencil = ds->stencil;
			front_failed = ToVulkanStencilOp(stencil->front.failed);
			front_pass = ToVulkanStencilOp(stencil->front.pass);
			front_depth_failed = ToVulkanStencilOp(stencil->front.depth_failed);
			front_compare = ToVulkanCompareOp(stencil->front.compare);

			back_failed = ToVulkanStencilOp(stencil->back.failed);
			back_pass = ToVulkanStencilOp(stencil->back.pass);
			back_depth_failed = ToVulkanStencilOp(stencil->back.depth_failed);
			back_compare = ToVulkanCompareOp(stencil->back.compare);

			stencil_read_mask = stencil->read_mask;
			stencil_write_mask = stencil->write_mask;

		}

		vk::PipelineDepthStencilStateCreateInfo depth_stencil_info(
			{},
			enable_depth,
			enable_depth_write,
			depth_compare,
			vk::False,		// depthBoundsTestEnable
			enable_stencil,	// stencilTestEnable
			{
				front_failed,
				front_pass,
				front_depth_failed,
				front_compare,
				stencil_read_mask,
				stencil_write_mask,
				0u
			},	// front
			{
				back_failed,
				back_pass,
				back_depth_failed,
				back_compare,
				stencil_read_mask,
				stencil_write_mask,
				0u
			},	// back
			0.0f, 	// minDepthBounds
			1.0f,	// maxDepthBounds
			nullptr	// pNext
		);

		auto& ms = declaration->multi_sample;
		vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1;
		vk::SampleMask mask = 0u;
		vk::Bool32 alpha_to_coverage_enable = vk::False;
		
		if (ms) {
			sample_count = SampleCountToFlag(ms->count);
			mask = static_cast<vk::SampleMask>(ms->mask);
			alpha_to_coverage_enable = ms->enable_alpha_to_coverage ? vk::True : vk::False;
		}

		vk::PipelineMultisampleStateCreateInfo multi_sample(
			{},
			sample_count,
			vk::False,
			0.0f,
			&mask,
			alpha_to_coverage_enable,
			vk::False,
			nullptr
		);

		auto& vertex_input_layout = declaration->vertex_input_layout;
		if (vertex_input_layout.bindings.empty()) {
			throw std::invalid_argument("Invalid vertex binding");
		}

		if (vertex_input_layout.attributes.empty()) {
			throw std::invalid_argument("Invalid vertex attributes");
		}

		std::vector<vk::VertexInputBindingDescription> bindings;
		std::vector<vk::VertexInputAttributeDescription> attributes;
		bindings.reserve(vertex_input_layout.bindings.size());
		attributes.reserve(vertex_input_layout.attributes.size());

		for (auto const& b : vertex_input_layout.bindings) {
			bindings.emplace_back(
				b.where, 
				b.stride, 
				b.input_rate == InputRate::Vertex ? vk::VertexInputRate::eVertex : vk::VertexInputRate::eInstance
			);
		}

		for (auto const& layout_attr : vertex_input_layout.attributes) {

			std::size_t location = std::visit(
				[](auto&& arg) -> std::uint32_t {
					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::same_as<T, std::size_t>) {
						return arg;
					} 
					else if constexpr (std::same_as<T, SemanticInfo>) {
						return arg.index;
					}
					else {
						throw std::invalid_argument("Invalid ID");
					}
				}, 
				layout_attr.id
			);

			attributes.emplace_back(
				static_cast<std::uint32_t>(location),
				static_cast<std::uint32_t>(layout_attr.where),
				DetermineFormat(layout_attr.format),
				static_cast<std::uint32_t>(layout_attr.offset)
			);

		}

		vk::PipelineVertexInputStateCreateInfo vertex_input_info(
			{},
			bindings,
			attributes,
			nullptr
		);

		vk::PipelineInputAssemblyStateCreateInfo input_assembly_info(
			{},
			ToVulkanPrimitiveTopology(flags),
			flags & GPUExecutableFlagBits::EnablePrimitiveRestart ? vk::True : vk::False,
			nullptr
		);

		std::array dynamic_states = {
			vk::DynamicState::eBlendConstants,
			vk::DynamicState::eDepthBounds,
			vk::DynamicState::eStencilReference,
			vk::DynamicState::eViewportWithCount,
			vk::DynamicState::eScissorWithCount
		};

		vk::PipelineDynamicStateCreateInfo dynamic_states_info({}, dynamic_states);

		if (declaration->rt_formats.empty()) {
			throw std::invalid_argument("Render target format not set");
		}

		std::vector<vk::Format> rt_formats;

		std::ranges::transform(
			declaration->rt_formats.begin(), 
			declaration->rt_formats.end(), 
			std::back_inserter(rt_formats),
			[](ResourceFlags flag) {
				return DetermineFormat(flag);
			}
		);

		auto&& [depth_format, stencil_format] = ds ? ToDepthStencilFormat(ds->format) : std::pair(vk::Format::eUndefined, vk::Format::eUndefined);

		vk::PipelineRenderingCreateInfo rendering_info(
			{},
			rt_formats,
			depth_format,
			stencil_format,
			nullptr
		);

		vk::GraphicsPipelineCreateInfo info(
			{},
			shaders,
			&vertex_input_info,
			&input_assembly_info,
			nullptr,
			nullptr,
			&rasterizer_info,
			&multi_sample,
			&depth_stencil_info,
			&blend_info,
			&dynamic_states_info,
			pipeline_layout,
			nullptr,
			0u,
			nullptr,
			0u,
			&rendering_info
		);

		m_handle.emplace<Compilation>(shader_data_segment_id, ShaderStage::Graphics, logical_device->CreatePipeline(nullptr, info));

	}

	void VulkanGPUExecutable::CompileComputingExecutable(Declaration* declaration) {

		VALIDATE_LOGICAL_DEVICE
		auto& shader_data_segment_id = declaration->shader_data_segment_id;
		if (!shader_data_segment_id) {
			throw std::invalid_argument("invalid shader data segment");
		}
		auto* shader_data_segment = plastic::utility::QueryObject<VulkanShaderDataSegment>(*shader_data_segment_id);
		if (!shader_data_segment) {
			throw std::runtime_error("shader data segment lost");
		}	

		vk::PipelineLayout pipeline_layout = shader_data_segment->GetPipelineLayout();

		VulkanShader* shader = nullptr;

		for (auto const& [stage, shader_id] : declaration->shaders) {

			if ((stage & ShaderStage::Compute) == ShaderStage::Unknown) {
				continue;
			}

			if (!shader_id) {
				throw std::invalid_argument("invalid shader");
			}
			shader = plastic::utility::QueryObject<VulkanShader>(*shader_id);
			if (!shader) {
				throw std::runtime_error("shader lost");
			}	
			
		}

		if (!shader) {
			throw std::invalid_argument("No compute shader");
		}

		vk::ComputePipelineCreateInfo info(
			{},
			{
				vk::PipelineShaderStageCreateFlags{},
				vk::ShaderStageFlagBits::eCompute,
				shader->GetNative(),
				"main",
				nullptr,
				nullptr
			},
			pipeline_layout,
			nullptr,
			0u
		);

		m_handle.emplace<Compilation>(shader_data_segment_id, ShaderStage::Compute, logical_device->CreatePipeline(nullptr, info));

	}

	void VulkanGPUExecutable::CompileMeshExecutable(Declaration *declaration)
	{
	}

	void VulkanGPUExecutable::CompileRayTracingExecutable(Declaration *declaration)
	{
	}

	VulkanGPUExecutable::VulkanGPUExecutable(VulkanLogicalDevice const& logical_device)
		: PolymorphicGPUExecutableBase(this),
		VulkanCommon(this),
		m_logical_device_id(logical_device.GetID()) {

	}

	VulkanGPUExecutable& VulkanGPUExecutable::SetShader(ShaderStage stage, VulkanShader const& shader) {
		
		std::visit(
			[stage, &shader](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, VulkanGPUExecutable::Declaration>) {
					if (HasConflictingFlags(stage, ShaderStage::All)) {
						throw std::invalid_argument("Invalid stage: Specifies multiple shader stages");
					}
					handle.shaders[stage] = shader.GetID();
				}
				else {
					throw std::logic_error("Cannot set after compilation");;
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
				if constexpr (std::same_as<Handle, VulkanGPUExecutable::Declaration>) {	\
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
	
	VulkanGPUExecutable& VulkanGPUExecutable::SetShaderDataSegment(VulkanShaderDataSegment const& shader_data_segment) {
		DECLARE_SINGLE(handle.shader_data_segment_id = shader_data_segment.GetID())
	}

	VulkanGPUExecutable& VulkanGPUExecutable::SetFlags(GPUExecutableFlags const& flags) {
		DECLARE_SINGLE(handle.flags = flags)
	}

	
	VulkanGPUExecutable& VulkanGPUExecutable::SetRenderTargetAttachments(RenderTargetAttachmentsInfo const& info) {
		DECLARE_SINGLE(handle.rt_att = info)
	}

	VulkanGPUExecutable& VulkanGPUExecutable::SetRenderTargetAttachments(std::span<BlendInfo const> blend_info, std::span<std::uint8_t const> color_write_masks) {
		
		std::visit(
			[blend_info, color_write_masks](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, VulkanGPUExecutable::Declaration>) {
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

	VulkanGPUExecutable& VulkanGPUExecutable::SetRenderTargetAttachments(LogicOp logic_op, std::span<std::uint8_t const> color_write_masks) {
		
		std::visit(
			[logic_op, color_write_masks](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, VulkanGPUExecutable::Declaration>) {
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

	VulkanGPUExecutable& VulkanGPUExecutable::SetDepthBias(DepthBias const &depth_bias) {
		DECLARE_SINGLE(handle.depth_bias.emplace(depth_bias))
	}

	VulkanGPUExecutable& VulkanGPUExecutable::SetDepthStencil(DepthStencilInfo const &depth_stencil) {
		DECLARE_SINGLE(handle.depth_stencil.emplace(depth_stencil))
	}

	VulkanGPUExecutable& VulkanGPUExecutable::SetMultiSample(MultiSample const &multi_sample) {
		DECLARE_SINGLE(handle.multi_sample.emplace(multi_sample))
	}

	VulkanGPUExecutable& VulkanGPUExecutable::SetRenderTargetFormats(std::span<ResourceFlags const> format) {
		DECLARE_SINGLE(handle.rt_formats.assign(format.begin(), format.end()))
	}

	VulkanGPUExecutable& VulkanGPUExecutable::SetVertexInputLayout(VertexInputLayout const& vertex_input_layout) {
		DECLARE_SINGLE(handle.vertex_input_layout = vertex_input_layout)
	}

	VulkanGPUExecutable& VulkanGPUExecutable::SetVertexInputLayout(std::span<VertexBinding const> bindings, std::span<VertexAttribute const> attributes) {
		std::visit(
			[bindings, attributes](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, VulkanGPUExecutable::Declaration>) {
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

	VulkanGPUExecutable& VulkanGPUExecutable::AddVertexInputLayout(VertexBinding const& binding, VertexAttribute const& attribute) {
		std::visit(
			[&binding, &attribute](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, VulkanGPUExecutable::Declaration>) {
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

	VulkanGPUExecutable& VulkanGPUExecutable::AddVertexBinding(VertexBinding const& binding) {
		std::visit(
			[&binding](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, VulkanGPUExecutable::Declaration>) {
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

	VulkanGPUExecutable& VulkanGPUExecutable::AddVertexAttribute(VertexAttribute const& attribute) {
		std::visit(
			[&attribute](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, VulkanGPUExecutable::Declaration>) {
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

#define ENSURE_COMPILATION	\
	auto* compilation = std::get_if<Compilation>(&m_handle);	\
	if (!compilation) return {};

	std::optional<std::size_t> VulkanGPUExecutable::GetAssociatedShaderDataSegmentID() const noexcept {
		ENSURE_COMPILATION
		return compilation->shader_data_segment_id;
	}

	std::pair<ShaderStage, vk::Pipeline> VulkanGPUExecutable::GetPSO() const noexcept {
		ENSURE_COMPILATION
		return { compilation->pso_type, *compilation->pso };
	}

	void VulkanGPUExecutable::Compile() {

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

	void VulkanGPUExecutable::Reset() {
		m_handle.emplace<Declaration>();
	}

} // namespace fyuu_rhi::vulkan


#endif // !defined(__APPLE__)