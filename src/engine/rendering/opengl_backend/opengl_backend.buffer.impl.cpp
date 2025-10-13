module;
#include <glad/glad.h>

module opengl_backend:buffer;
import :command_object;

namespace fyuu_engine::opengl {

	OpenGLBuffer::OpenGLBuffer(OpenGLCommandObject* command_object, GLenum target, GLenum usage, std::size_t size)
		: m_command_object(command_object), m_target(target), m_usage(usage), m_size(size) {
		m_command_object->Execute(
			[this]() {
				glGenBuffers(1, &m_id);
				glBindBuffer(m_target, m_id);
				glBufferData(m_target, m_size, nullptr, m_usage);
			}
		);
	}

	OpenGLBuffer::~OpenGLBuffer() noexcept {
		m_command_object->Execute(
			[id = m_id]() {
				glDeleteBuffers(1, &id);
			}
		);
	}

	void OpenGLBuffer::Update(std::span<std::byte> data) {
		m_command_object->Execute(
			[data, target = m_target, usage = m_usage, size = m_size, id = m_id]() {
				glBindBuffer(GL_ARRAY_BUFFER, id);
				data.size() > size ?
					glBufferData(target, data.size(), data.data(), usage) :
					glBufferSubData(target, 0, data.size(), data.data());
				
			}
		);
	}

	GLenum OpenGLBuffer::GetTarget() const noexcept {
		return m_target;
	}

	GLuint OpenGLBuffer::GetID() const noexcept {
		return m_id;
	}

	void OpenGLBufferCollector::operator()(OpenGLBuffer const& buffer)
	{
	}

}