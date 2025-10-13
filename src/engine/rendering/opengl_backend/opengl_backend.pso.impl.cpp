module;
#include <glad/glad.h>

module opengl_backend:pso;
import file;

namespace fs = std::filesystem;

namespace fyuu_engine::opengl {

	/*
	*	OpenGLPipelineStateObjectBuilder implementations
	*/

	GLuint OpenGLPipelineStateObjectBuilder::CompileShader(GLenum type, std::span<std::byte> shader_source) {

		GLuint shader = glCreateShader(type);

		char const* str_src = reinterpret_cast<char const*>(shader_source.data());
		GLint str_len = static_cast<GLint>(shader_source.size());
		glShaderSource(shader, 1, &str_src, &str_len);
		glCompileShader(shader);

		GLint success = 0;
		std::array<char, 512> info_log{};
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			throw std::runtime_error(info_log.data());
		}

		return shader;

	}

	concurrency::AsyncTask<OpenGLPipelineStateObject> OpenGLPipelineStateObjectBuilder::BuildImpl() const {
		co_return OpenGLPipelineStateObject{};
	}

	OpenGLPipelineStateObjectBuilder& OpenGLPipelineStateObjectBuilder::LoadVertexShaderImpl(std::filesystem::path const& path) {
		throw std::runtime_error("Traditional GLSL does not support load existing shader, use modern SPIR-V instead");
	}

	OpenGLPipelineStateObjectBuilder& OpenGLPipelineStateObjectBuilder::LoadFragmentShaderImpl(std::filesystem::path const& path) {
		throw std::runtime_error("Traditional GLSL does not support load existing shader, use modern SPIR-V instead");
	}

	OpenGLPipelineStateObjectBuilder& OpenGLPipelineStateObjectBuilder::LoadPixelShaderImpl(std::filesystem::path const& path) {
		return LoadFragmentShaderImpl(path);
	}

	OpenGLPipelineStateObjectBuilder& OpenGLPipelineStateObjectBuilder::CompileVertexShaderImpl(
		std::filesystem::path const& path, 
		std::string_view entry
	) {
		m_command_object->Execute(
			[=]() {
				auto source = util::ReadFile(path);
				m_vertex_shader = CompileShader(GL_VERTEX_SHADER, source);
			}
		);
		return *this;
	}

	OpenGLPipelineStateObjectBuilder& OpenGLPipelineStateObjectBuilder::CompileFragmentShaderImpl(
		std::filesystem::path const& path, 
		std::string_view entry
	) {
		m_command_object->Execute(
			[=]() {
				auto source = util::ReadFile(path);
				m_vertex_shader = CompileShader(GL_FRAGMENT_SHADER, source);
			}
		);
		return *this;
	}

	OpenGLPipelineStateObjectBuilder& OpenGLPipelineStateObjectBuilder::CompilePixelShaderImpl(
		std::filesystem::path const& path, 
		std::string_view entry
	) {
		return *this;
	}

	OpenGLPipelineStateObjectBuilder::OpenGLPipelineStateObjectBuilder(OpenGLCommandObject& command_object)
		: Base(NoSchedulerFlag()),
		m_command_object(&command_object) {
	}

	/*
	*	OpenGLSPIRVPipelineStateObjectBuilder implementations
	*/

	GLuint OpenGLSPIRVPipelineStateObjectBuilder::LoadShader(
		GLenum type, 
		std::span<std::byte> shader_source
	) {

		/*
		*	load shader
		*/

		GLuint shader = glCreateShader(type);
		glShaderBinary(
			1, &shader,
			GL_SHADER_BINARY_FORMAT_SPIR_V_ARB,
			shader_source.data(), static_cast<GLsizei>(shader_source.size())
		);

		/*
		*	specialize shader
		*/

		glSpecializeShaderARB(shader, "main", 0, nullptr, nullptr);

		/*
		*	check compilation error
		*/

		GLint success = 0;
		std::array<char, 512> info_log{};
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			throw std::runtime_error(info_log.data());
		}

		/*
		*	create shader program
		*/

		GLuint shader_program = glCreateProgram();
		glAttachShader(shader_program, shader);
		glLinkProgram(shader_program);

		/*
		*	check link error
		*/

		glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
		if (!success) {
			throw std::runtime_error(info_log.data());
		}

		glDeleteShader(shader);

		return shader_program;

	}

	concurrency::AsyncTask<OpenGLPipelineStateObject> OpenGLSPIRVPipelineStateObjectBuilder::BuildImpl() const {
		co_return OpenGLPipelineStateObject{};
	}

	OpenGLSPIRVPipelineStateObjectBuilder& OpenGLSPIRVPipelineStateObjectBuilder::LoadVertexShaderImpl(
		std::filesystem::path const& path
	) {
		m_command_object->Execute(
			[=]() {
				std::vector<std::byte> byte_code = util::ReadFile(path);
				m_vertex_shader = LoadShader(GL_VERTEX_SHADER, byte_code);
			}
		);
		return *this;
	}

	OpenGLSPIRVPipelineStateObjectBuilder& OpenGLSPIRVPipelineStateObjectBuilder::LoadFragmentShaderImpl(
		std::filesystem::path const& path
	) {
		m_command_object->Execute(
			[=]() {
				//std::vector<std::byte> byte_code = util::ReadFile(path);
				//m_fragment_shader = LoadShader(GL_FRAGMENT_SHADER, byte_code);
			}
		);
		return *this;
	}

	OpenGLSPIRVPipelineStateObjectBuilder& OpenGLSPIRVPipelineStateObjectBuilder::LoadPixelShaderImpl(
		std::filesystem::path const& path
	) {
		return LoadFragmentShaderImpl(path);
	}

	OpenGLSPIRVPipelineStateObjectBuilder& OpenGLSPIRVPipelineStateObjectBuilder::CompileVertexShaderImpl(
		std::filesystem::path const& path,
		std::string_view entry
	) {
		m_command_object->Execute(
			[=]() {
				//auto source = util::ReadFile(path);
				//m_vertex_shader = CompileShader(GL_VERTEX_SHADER, source);
			}
		);
		return *this;
	}

	OpenGLSPIRVPipelineStateObjectBuilder& OpenGLSPIRVPipelineStateObjectBuilder::CompileFragmentShaderImpl(
		std::filesystem::path const& path,
		std::string_view entry
	) {
		m_command_object->Execute(
			[=]() {
				//auto source = util::ReadFile(path);
				//m_vertex_shader = CompileShader(GL_FRAGMENT_SHADER, source);
			}
		);
		return *this;
	}

	OpenGLSPIRVPipelineStateObjectBuilder& OpenGLSPIRVPipelineStateObjectBuilder::CompilePixelShaderImpl(
		std::filesystem::path const& path,
		std::string_view entry
	) {
		return *this;
	}


	OpenGLSPIRVPipelineStateObjectBuilder::OpenGLSPIRVPipelineStateObjectBuilder(OpenGLCommandObject& command_object)
		: Base(NoSchedulerFlag()),
		m_command_object(&command_object) {
	}

}