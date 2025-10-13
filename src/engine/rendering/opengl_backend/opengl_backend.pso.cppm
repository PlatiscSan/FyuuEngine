module;
#include <glad/glad.h>

export module opengl_backend:pso;
export import rendering;
export import :command_object;
import std;

namespace fyuu_engine::opengl {

	export struct OpenGLPipelineStateObject {

	};

	export class OpenGLPipelineStateObjectBuilder 
		: public core::BasePipelineStateObjectBuilder<OpenGLPipelineStateObjectBuilder, OpenGLPipelineStateObject> {
		friend class core::BasePipelineStateObjectBuilder<OpenGLPipelineStateObjectBuilder, OpenGLPipelineStateObject>;
	private:
		OpenGLCommandObject* m_command_object;
		GLuint m_vertex_shader;
		GLuint m_fragment_shader;

		static GLuint CompileShader(GLenum type, std::span<std::byte> shader_source);

		concurrency::AsyncTask<OpenGLPipelineStateObject> BuildImpl() const;

		OpenGLPipelineStateObjectBuilder& LoadVertexShaderImpl(std::filesystem::path const& path);

		OpenGLPipelineStateObjectBuilder& LoadFragmentShaderImpl(std::filesystem::path const& path);

		OpenGLPipelineStateObjectBuilder& LoadPixelShaderImpl(std::filesystem::path const& path);

		OpenGLPipelineStateObjectBuilder& CompileVertexShaderImpl(std::filesystem::path const& path, std::string_view entry);

		OpenGLPipelineStateObjectBuilder& CompileFragmentShaderImpl(std::filesystem::path const& path, std::string_view entry);

		OpenGLPipelineStateObjectBuilder& CompilePixelShaderImpl(std::filesystem::path const& path, std::string_view entry);

	public:
		using Base = core::BasePipelineStateObjectBuilder<OpenGLPipelineStateObjectBuilder, OpenGLPipelineStateObject>;
		OpenGLPipelineStateObjectBuilder(OpenGLCommandObject& command_object);
	};

	export class OpenGLSPIRVPipelineStateObjectBuilder 
		: public core::BasePipelineStateObjectBuilder<OpenGLSPIRVPipelineStateObjectBuilder, OpenGLPipelineStateObject> {
		friend class core::BasePipelineStateObjectBuilder<OpenGLSPIRVPipelineStateObjectBuilder, OpenGLPipelineStateObject>;
	private:
		OpenGLCommandObject* m_command_object;
		GLuint m_vertex_shader;
		GLuint m_fragment_shader;

		static GLuint LoadShader(GLenum type, std::span<std::byte> shader_source);



		concurrency::AsyncTask<OpenGLPipelineStateObject> BuildImpl() const;

		OpenGLSPIRVPipelineStateObjectBuilder& LoadVertexShaderImpl(std::filesystem::path const& path);

		OpenGLSPIRVPipelineStateObjectBuilder& LoadFragmentShaderImpl(std::filesystem::path const& path);

		OpenGLSPIRVPipelineStateObjectBuilder& LoadPixelShaderImpl(std::filesystem::path const& path);

		OpenGLSPIRVPipelineStateObjectBuilder& CompileVertexShaderImpl(std::filesystem::path const& path, std::string_view entry);

		OpenGLSPIRVPipelineStateObjectBuilder& CompileFragmentShaderImpl(std::filesystem::path const& path, std::string_view entry);

		OpenGLSPIRVPipelineStateObjectBuilder& CompilePixelShaderImpl(std::filesystem::path const& path, std::string_view entry);

	public:
		using Base = core::BasePipelineStateObjectBuilder<OpenGLSPIRVPipelineStateObjectBuilder, OpenGLPipelineStateObject>;
		OpenGLSPIRVPipelineStateObjectBuilder(OpenGLCommandObject& command_object);
	};

}