module;
#include <glad/glad.h>

module graphics:base_opengl_renderer;

namespace graphics::api::opengl {

	void BaseOpenGLRenderDevice::OnCommandReady(CommandReadyEvent const& e) {

		while (!m_allow_submission.test(std::memory_order::acquire)) {
			/*
			*	Wait until the present thread allow to submit again
			*/
			m_allow_submission.wait(false, std::memory_order::relaxed);
		}

		FrameContext& next_frame = m_frames[m_next_frame_index];
		{

			std::lock_guard<std::mutex> lock(next_frame.ready_commands_mutex);
			next_frame.ready_commands.append_range(std::move(e.commands));
		}

	}

	bool BaseOpenGLRenderDevice::WaitForGPU(FrameContext& frame) {

		if (!frame.fence) {
			return true;
		}

		GLenum result = glClientWaitSync(
			frame.fence,
			GL_SYNC_FLUSH_COMMANDS_BIT,
			std::numeric_limits<std::uint64_t>::max()
		);
		glDeleteSync(frame.fence);
		frame.fence = nullptr;

		switch (result) {
		case GL_ALREADY_SIGNALED:
		case GL_CONDITION_SATISFIED:
			return true;
		case GL_TIMEOUT_EXPIRED:
			m_logger->Warning(
				std::source_location::current(),
				"Frame synchronization timeout"
			);
			return false;
		case GL_WAIT_FAILED:
			m_logger->Error(
				std::source_location::current(),
				"glClientWaitSync() failed"
			);
			return false;
		default:
			return false;
		}

	}

	bool BaseOpenGLRenderDevice::WaitForNextFrame() {
		return WaitForGPU(m_frames[m_next_frame_index]);
	}

	bool BaseOpenGLRenderDevice::WaitForPreviousFrame() {
		return WaitForGPU(m_frames[m_previous_frame_index]);
	}

	BaseOpenGLRenderDevice::BaseOpenGLRenderDevice(BaseOpenGLRenderDevice&& other) noexcept 
		: BaseRenderDevice(std::move(other)),
		m_frames(std::move(other.m_frames)),
		m_command_ready_subscription(
			m_message_bus.Subscribe<CommandReadyEvent>(
				[this](CommandReadyEvent const& e) {
					OnCommandReady(e);
				}
			)
		) {

		/*
		*	remove previous subscription
		*/

		m_message_bus.Unsubscribe(other.m_command_ready_subscription);
		other.m_command_ready_subscription = nullptr;

	}

	BaseOpenGLRenderDevice& BaseOpenGLRenderDevice::operator=(BaseOpenGLRenderDevice&& other) noexcept {
		if (this != &other) {
			
			static_cast<BaseRenderDevice&&>(*this) = std::move(static_cast<BaseRenderDevice&&>(other));
			m_frames = std::move(other.m_frames);
			m_command_ready_subscription = m_message_bus.Subscribe<CommandReadyEvent>(
				[this](CommandReadyEvent const& e) {
					OnCommandReady(e);
				}
			);

			/*
			*	remove previous subscription
			*/
			m_message_bus.Unsubscribe(other.m_command_ready_subscription);
			other.m_command_ready_subscription = nullptr;
				
		}
		return *this;
	}

	void BaseOpenGLRenderDevice::Clear(float r, float g, float b, float a) {
		auto& command_object = static_cast<OpenGLCommandObject&>(AcquireCommandObject());
		command_object.SubmitCommand(
			[r, g, b, a]() {
				glClearColor(r, g, b, a);
				glClear(GL_COLOR_BUFFER_BIT);
			}
		);
	}

	void BaseOpenGLRenderDevice::SetViewport(int x, int y, std::uint32_t width, std::uint32_t height) {
		auto& command_object = static_cast<OpenGLCommandObject&>(AcquireCommandObject());
		command_object.SubmitCommand(
			[x, y, width, height]() {
				glViewport(x, y, width, height);
			}
		);
	}

	bool BaseOpenGLRenderDevice::BeginFrame() {

		bool should_proceed = 
			!m_is_minimized && 
			m_is_window_alive &&
			WaitForNextFrame();

		if (!should_proceed) {
			return false;
		}

		// command object for the present thread
		auto& command_object = static_cast<OpenGLCommandObject&>(AcquireCommandObject());

		/*
		*	Start present thread recording
		*/

		command_object.StartRecording();

		/*
		*	notify other awaiting rendering thread
		*/

		m_allow_submission.test_and_set(std::memory_order::release);
		m_allow_submission.notify_all();

		return true;

	}

	void BaseOpenGLRenderDevice::EndFrame() {

		FrameContext& next_frame = m_frames[m_next_frame_index];

		// command object for the present thread
		auto& command_object = static_cast<OpenGLCommandObject&>(AcquireCommandObject());

		/*
		*	End the recording in the present thread.
		*/

		command_object.EndRecording();

		/*
		*	No more submission to this frame
		*/

		m_allow_submission.clear(std::memory_order::release);
		
		{
			/*
			*	Execute all commands
			*/
			std::lock_guard<std::mutex> lock(next_frame.ready_commands_mutex);
			for (auto const& command : next_frame.ready_commands) {
				command();
			}
			next_frame.ready_commands.clear();
		}

		glFlush();
		SwapBuffer();

		next_frame.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		m_previous_frame_index = m_next_frame_index;
		m_next_frame_index = (m_next_frame_index + 1) % m_frames.size();

	}

	API BaseOpenGLRenderDevice::GetAPI() const noexcept {
		return API::OpenGL;
	}

	ICommandObject& BaseOpenGLRenderDevice::AcquireCommandObject() {
		static thread_local std::optional<OpenGLCommandObject> command_object;
		static thread_local std::once_flag command_objects_initialized;
		std::call_once(
			command_objects_initialized,
			[this]() {
				command_object.emplace(&m_message_bus);
			}
		);
		return *command_object;
	}

}
