module;
#include <glad/glad.h>

module opengl_backend:renderer;

namespace fyuu_engine::opengl {
	void LoggerCallback(
		GLenum source, GLenum type, unsigned int id, GLenum severity, 
		GLsizei length, char const* message, void const* user_param
	) {

		auto logger = const_cast<core::IRendererLogger*>(static_cast<core::IRendererLogger const*>(user_param));

		/*
		*	Ignore some unimportant debugging information
		*/
		if (id == 131169 || id == 131185 || id == 131218 || id == 131204) 
			return;

		char const* source_str;

		switch (source) {
		case GL_DEBUG_SOURCE_API:             source_str = "API\0"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   source_str = "Window System\0"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: source_str = "Shader Compiler\0"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     source_str = "Third Party\0"; break;
		case GL_DEBUG_SOURCE_APPLICATION:     source_str = "Application\0"; break;
		case GL_DEBUG_SOURCE_OTHER:           source_str = "Other\0"; break;
		}

		core::LogLevel level;
		char const* type_str;

		switch (type) {
		case GL_DEBUG_TYPE_ERROR:               
			level = core::LogLevel::Error;
			type_str = "Error\0";
			break;

		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			level = core::LogLevel::Warning;
			type_str = "Deprecated Behaviour\0";
			break;

		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  
			level = core::LogLevel::Error;
			type_str = "Undefined Behaviour\0";
			break;

		case GL_DEBUG_TYPE_PORTABILITY:
			level = core::LogLevel::Info;
			type_str = "Portability\0";
			break;

		case GL_DEBUG_TYPE_PERFORMANCE:
			level = core::LogLevel::Info;
			type_str = "Performance\0";
			break;

		case GL_DEBUG_TYPE_MARKER:
			level = core::LogLevel::Info;
			type_str = "Marker\0";
			break;

		case GL_DEBUG_TYPE_PUSH_GROUP:
			level = core::LogLevel::Info;
			type_str = "Push Group\0";
			break;

		case GL_DEBUG_TYPE_POP_GROUP:
			level = core::LogLevel::Info;
			type_str = "Pop Group\0";
			break;

		case GL_DEBUG_TYPE_OTHER:
			level = core::LogLevel::Info;
			type_str = "Other\0";
			break;

		default:
			level = core::LogLevel::Debug;
			type_str = "Other\0";
			break;
		}

		std::string formatted_message = std::format("OpenGL source:{}, type:{}", source_str, type_str);

		logger->Log(level, formatted_message);

	}

	GLenum GetBufferTarget(core::BufferType type) noexcept {
		switch (type) {
		case core::BufferType::Vertex:
			return GL_ARRAY_BUFFER;

		case core::BufferType::Index:
			return GL_ELEMENT_ARRAY_BUFFER;

		case core::BufferType::Uniform:
			return GL_UNIFORM_BUFFER;

		case core::BufferType::Upload:
			return GL_ARRAY_BUFFER;

		case core::BufferType::Staging:
			return GL_COPY_READ_BUFFER;

		default:
			return GL_ARRAY_BUFFER;
		}
	}

	GLenum GetBufferUsage(core::BufferType type) noexcept {
		switch (type) {
		case core::BufferType::Vertex:
		case core::BufferType::Index:
		case core::BufferType::Uniform:
			return GL_STATIC_DRAW;

		case core::BufferType::Upload:
			return GL_DYNAMIC_DRAW;

		case core::BufferType::Staging:
			return GL_DYNAMIC_READ;

		default:
			return GL_STATIC_DRAW;
		}
	}

}
