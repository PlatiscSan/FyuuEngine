// opengl_shader_data_segment.cppm
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <vector>
#include <unordered_map>
#include <variant>
#include <optional>
#endif // !defined(__cpp_lib_modules)
#include "glad.hpp"
export module fyuu_rhi:opengl_shader_data_segment;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import :types;
import :opengl_common;
import :opengl_sampler;

namespace fyuu_rhi::opengl {
		
	// A simple struct to represent push constant ranges in OpenGL
	// (used for consistency with Vulkan; may be expanded if needed)
	struct PushConstantRange {
		std::size_t offset;
		std::size_t size;
		ShaderStage stages;
	};

	export struct BindingInfo {
		std::size_t where;					 								// binding point
		std::variant<std::monostate, ResourceFlags, SamplerFlags> flags; 	// type info
		std::size_t count;
		ShaderStage stages;
	};

	export class OpenGLShaderDataSegment
		: public PolymorphicShaderDataSegmentBase,
		public OpenGLCommon<OpenGLShaderDataSegment> {
	private:
		// Declaration types (same as before)
		struct Declaration {
			struct PushConstant {
				std::size_t count;
			};
			struct Binding {
				std::variant<std::monostate, ResourceFlags, SamplerFlags> flags;
				std::size_t count;
			};
			struct StaticSamplerDeclaration {
				SamplerFlags flags;
				SamplerAttachmentInfo attachment;
			};
			std::size_t where;
			std::variant<PushConstant, Binding, StaticSamplerDeclaration> info;
			ShaderStage stages;
			std::size_t ns;		   // namespace (set) identifier, added for consistency
		};

		// Instantiated data (similar to Vulkan's)
		struct InstantiatedData {
			struct SetLayout {
				std::vector<BindingInfo> bindings;
				std::unordered_map<std::size_t, OpenGLSampler> immutable_samplers; // map binding -> sampler
			};
			std::unordered_map<std::size_t, SetLayout> set_layouts; // namespace -> layout
			std::vector<PushConstantRange> push_constants;
		};

		// State: either collecting declarations or already instantiated
		std::optional<std::size_t> m_logical_device_id;
		std::variant<std::vector<Declaration>, InstantiatedData> m_handle;

	public:
		// Info structure for immutable samplers (matching Vulkan's)
		struct ImmutableSamplerInfo {
			std::uint32_t set;
			std::uint32_t binding;
			OpenGLSampler const* impl;
		};

		OpenGLShaderDataSegment(OpenGLLogicalDevice const& logical_device);

		// Declare methods (now with ns parameter)
		OpenGLShaderDataSegment& Declare(
			std::size_t where, 
			std::size_t count,
			ShaderStage visible, 
			std::size_t ns = 0u
		);

		OpenGLShaderDataSegment& Declare(
			ResourceFlags flags, 
			std::size_t where,			 
			ShaderStage visible, 
			std::size_t ns = 0u
		);

		OpenGLShaderDataSegment& Declare(
			ResourceFlags flags, 
			std::size_t where,
			std::size_t count, 
			ShaderStage visible,
			std::size_t ns = 0u
		);

		OpenGLShaderDataSegment& Declare(
			SamplerFlags flags, 
			SamplerAttachmentInfo const& info,
			std::size_t where, 
			ShaderStage visible,
			std::size_t ns = 0u
		);

		OpenGLShaderDataSegment& Declare(
			SamplerFlags flags, 
			std::size_t where,
			std::size_t count, ShaderStage visible,
			std::size_t ns = 0u
		);
			
		void Instantiate();
		void Reset();

		// Query methods
		std::pair<BindingInfo const*, OpenGLSampler const*> GetBindingInfo(std::size_t ns, std::size_t where) const noexcept;

		std::vector<ImmutableSamplerInfo> GetImmutableSamplers() const;

	};

} // namespace fyuu_rhi::opengl

#endif // !defined(__APPLE__)