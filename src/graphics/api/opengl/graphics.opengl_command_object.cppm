module;
#ifdef WIN32
#include <Windows.h>
#endif // WIN32
#include <glad/glad.h>

#ifdef interface
#undef interface
#endif // interface

export module graphics:opengl_command_object;
export import :interface;
import std;

namespace graphics::api::opengl {

	struct CommandReadyEvent {
		std::vector<std::function<void()>>& commands;
	};

	export class OpenGLCommandObject final : public ICommandObject {
	private:
		util::MessageBusPtr m_message_bus;
		std::vector<std::function<void()>> m_commands;
		bool m_is_recording;

	public:
		OpenGLCommandObject(util::MessageBusPtr const& message_bus);
		OpenGLCommandObject(OpenGLCommandObject&& other) noexcept;
		OpenGLCommandObject& operator=(OpenGLCommandObject&& other) noexcept;

		~OpenGLCommandObject() noexcept override = default;
		void* GetNativeHandle() noexcept override;
		API GetAPI() const noexcept override;
		void StartRecording() override;
		void EndRecording() override;
		void Reset() override;

		template <std::convertible_to<std::function<void()>> Command>
		void SubmitCommand(Command&& command) {
			m_commands.emplace_back(std::forward<Command>(command));
		}

	};

}