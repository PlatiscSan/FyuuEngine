module;
#ifdef WIN32
#include <Windows.h>
#endif // WIN32
#include <glad/glad.h>

module graphics:opengl_command_object;

namespace graphics::api::opengl {

	OpenGLCommandObject::OpenGLCommandObject(util::MessageBusPtr const& message_bus)
		: m_message_bus(util::MakeReferred(message_bus)),
		m_commands() {
	}

	OpenGLCommandObject::OpenGLCommandObject(OpenGLCommandObject&& other) noexcept
		: m_message_bus(std::move(other.m_message_bus)),
		m_commands(std::move(other.m_commands)),
		m_is_recording(std::exchange(other.m_is_recording, false)) {
	}

	OpenGLCommandObject& OpenGLCommandObject::operator=(OpenGLCommandObject&& other) noexcept {
		if (this != &other) {
			m_message_bus = std::move(other.m_message_bus);
			m_commands = std::move(other.m_commands);
			m_is_recording = std::exchange(other.m_is_recording, false);
		}
		return *this;
	}

	void* OpenGLCommandObject::GetNativeHandle() noexcept {
		return &m_commands;
	}

	API OpenGLCommandObject::GetAPI() const noexcept {
		return API::OpenGL;
	}

	void OpenGLCommandObject::StartRecording() {

		if (m_is_recording) {
			return;
		}

		m_commands.clear();
		m_is_recording = true;

	}

	void OpenGLCommandObject::EndRecording() {

		if (!m_is_recording) {
			return;
		}

		if (m_message_bus) {
			CommandReadyEvent e{ m_commands };
			m_message_bus->Publish(e);
		}

		m_is_recording = false;

	}

	void OpenGLCommandObject::Reset() {
		m_commands.clear();
	}

}