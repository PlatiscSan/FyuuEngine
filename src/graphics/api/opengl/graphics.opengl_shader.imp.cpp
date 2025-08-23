module;
#include <GL/glcorearb.h>

module graphics:opengl_shader;

#ifdef WIN32
static PFNGLCREATESHADERPROC glCreateShader;
static PFNGLSHADERSOURCEPROC glShaderSource;
static PFNGLCOMPILESHADERPROC glCompileShader;
static PFNGLGETSHADERIVPROC glGetShaderiv;
static PFNGLDELETESHADERPROC glDeleteShader;
static PFNGLUSEPROGRAMPROC glUseProgram;
#endif // WIN32


namespace graphics::api::opengl {

	static GLenum GeneratorShaderID(ShaderType type) {

		static std::once_flag once;
		std::call_once(
			once,
			[]() {
#ifdef WIN32
				glCreateShader = reinterpret_cast<PFNGLCREATESHADERPROC>(wglGetProcAddress("glCreateShader"));
				glShaderSource = reinterpret_cast<PFNGLSHADERSOURCEPROC>(wglGetProcAddress("glShaderSource"));
				glCompileShader = reinterpret_cast<PFNGLCOMPILESHADERPROC>(wglGetProcAddress("glCompileShader"));
				glGetShaderiv = reinterpret_cast<PFNGLGETSHADERIVPROC>(wglGetProcAddress("glGetShaderiv"));
				glDeleteShader = reinterpret_cast<PFNGLDELETESHADERPROC>(wglGetProcAddress("glDeleteShader"));
				glUseProgram = reinterpret_cast<PFNGLUSEPROGRAMPROC>(wglGetProcAddress("glUseProgram"));
#endif // WIN32
			}
		);

		switch (type) {
		case ShaderType::VERTEX:
			return GL_VERTEX_SHADER;
		case ShaderType::PIXEL:
			return GL_FRAGMENT_SHADER;
		case ShaderType::GEOMETRY:
			return GL_GEOMETRY_SHADER;
		case ShaderType::COMPUTE:
			return GL_COMPUTE_SHADER;
		default:
			throw std::invalid_argument("Unknown shader type");
		}

	}

	OpenGLShader::OpenGLShader(std::string_view source, ShaderType type)
		: m_source(source), m_type(type), m_shader_id(glCreateShader(GeneratorShaderID(m_type))) {

		char const* src = m_source.data();

		glShaderSource(m_shader_id, 1, &src, nullptr);
		glCompileShader(m_shader_id);

		GLint success;
		glGetShaderiv(m_shader_id, GL_COMPILE_STATUS, &success);

		if (success != GL_TRUE) {
			throw std::runtime_error("Shader compilation error");
		}

	}

	OpenGLShader::~OpenGLShader() noexcept {
		glDeleteShader(m_shader_id);
	}

	ShaderType OpenGLShader::GetType() const noexcept {
		return m_type;
	}

	ShaderFormat OpenGLShader::GetFormat() const noexcept {
		return ShaderFormat::GLSL;
	}

	std::string_view OpenGLShader::GetSource() const noexcept {
		return m_source;
	}

	std::string_view  OpenGLShader::GetEntryPoint() const noexcept {
		return "";
	}

	void OpenGLShader::Bind(BaseRenderDevice& renderer) const noexcept {
		glUseProgram(m_shader_id);
	}

	void OpenGLShader::Unbind() const noexcept {
		glUseProgram(0);
	}

	void* OpenGLShader::GetNativeHandle() const noexcept {
		return reinterpret_cast<void*>(static_cast<uintptr_t>(m_shader_id));
	}

}