module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <unordered_map>
#include <vector>
#include <variant>
#include <optional>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "glad.hpp"
export module fyuu_rhi:opengl_gpu_executable;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :enums;
import :types;
import :opengl_common;

namespace fyuu_rhi::opengl {
	
	export class OpenGLGPUExecutable
		: public PolymorphicGPUExecutableBase,
		public OpenGLCommon<OpenGLGPUExecutable> {
	private:
		struct Declaration {
			std::unordered_map<ShaderStage, std::optional<std::size_t>> shaders;
			std::optional<std::size_t> shader_data_segment_id;
			GPUExecutableFlags flags;
			RenderTargetAttachmentsInfo rt_att;
			std::optional<DepthBias> depth_bias;
			std::optional<DepthStencilInfo> depth_stencil;
			std::optional<MultiSample> multi_sample;
			VertexInputLayout vertex_input_layout;
			std::vector<ResourceFlags> rt_formats;
		};
		
		struct Compilation : OpenGLCommon<Compilation> {
			std::unordered_map<ShaderStage, GLuint> stage_programs;
			std::vector<GLenum> rt_formats;
			std::optional<std::size_t> shader_data_segment_id;
			ShaderStage pso_type;
			GLenum topology;
			GLuint pipeline;

			Compilation(OpenGLGPUExecutable* execu);
			Compilation(Compilation&& other) noexcept = default;
			~Compilation() noexcept;
		};

		std::variant<Declaration, Compilation> m_handle;

		// Compilation helpers
		void CompileGraphicsExecutable(Declaration* declaration);
		void CompileComputingExecutable(Declaration* declaration);
		void CompileMeshExecutable(Declaration* declaration);
		void CompileRayTracingExecutable(Declaration* declaration);

	public:
		OpenGLGPUExecutable(OpenGLLogicalDevice const& logical_device);
		OpenGLGPUExecutable(OpenGLGPUExecutable&& other) noexcept = default;

		// Builder methods (only valid in Declaration state)
		OpenGLGPUExecutable& SetShader(ShaderStage stage, OpenGLShader const& shader);
		OpenGLGPUExecutable& SetShaderDataSegment(OpenGLShaderDataSegment const& shader_data_segment);
		OpenGLGPUExecutable& SetFlags(GPUExecutableFlags const& flags);
		OpenGLGPUExecutable& SetRenderTargetAttachments(RenderTargetAttachmentsInfo const& info);
		OpenGLGPUExecutable& SetRenderTargetAttachments(std::span<BlendInfo const> blend_info, std::span<std::uint8_t const> color_write_masks);
		OpenGLGPUExecutable& SetRenderTargetAttachments(LogicOp logic_op, std::span<std::uint8_t const> color_write_masks);
		OpenGLGPUExecutable& SetDepthBias(DepthBias const& depth_bias);
		OpenGLGPUExecutable& SetDepthStencil(DepthStencilInfo const& depth_stencil);
		OpenGLGPUExecutable& SetMultiSample(MultiSample const& multi_sample);
		OpenGLGPUExecutable& SetRenderTargetFormats(std::span<ResourceFlags const> format);
		OpenGLGPUExecutable& SetVertexInputLayout(VertexInputLayout const& vertex_input_layout);
		OpenGLGPUExecutable& SetVertexInputLayout(std::span<VertexBinding const> bindings, std::span<VertexAttribute const> attributes);
		OpenGLGPUExecutable& AddVertexInputLayout(VertexBinding const& binding, VertexAttribute const& attribute);
		OpenGLGPUExecutable& AddVertexBinding(VertexBinding const& binding);
		OpenGLGPUExecutable& AddVertexAttribute(VertexAttribute const& attribute);

		// State queries (only valid in Compilation state)
		std::optional<std::size_t> GetAssociatedShaderDataSegmentID() const noexcept;
		std::pair<ShaderStage, GLuint> GetPSO() const noexcept;

		// Compilation and reset
		void Compile();
		void Reset();

		// Pipeline binding (OpenGL specific)
		void Bind();
		void Unbind() noexcept;

		GLenum GetTopology() const noexcept;

		GLuint GetProgram(ShaderStage stage) const noexcept;
		
	};

} // namespace fyuu_rhi::opengl

#endif // !defined(__APPLE__)