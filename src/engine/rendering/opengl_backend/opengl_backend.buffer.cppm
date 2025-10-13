module;
#include <glad/glad.h>

export module opengl_backend:buffer;
export import collective_resource;
import std;

namespace fyuu_engine::opengl {

	export class OpenGLCommandObject;

	export class OpenGLBuffer {
	private:
		OpenGLCommandObject* m_command_object;
		GLenum m_target;
		GLenum m_usage;
		std::size_t m_size;
		GLuint m_id;

	public:
		OpenGLBuffer(OpenGLCommandObject* command_object, GLenum target, GLenum usage, std::size_t size);
		~OpenGLBuffer() noexcept;
		void Update(std::span<std::byte> data);
		GLenum GetTarget() const noexcept;
		GLuint GetID() const noexcept;
	};

	export struct OpenGLBufferCollector {
		void operator()(OpenGLBuffer const& buffer);
	};

	export using UniqueOpenGLBuffer = util::UniqueCollectiveResource<OpenGLBuffer, OpenGLBufferCollector>;

}