module;
#include <glad/glad.h>

export module graphics:base_opengl_renderer;
export import :opengl_command_object;
import std;
import defer;

namespace graphics::api::opengl {

	struct FrameContext {
		GLsync fence;
		std::vector<std::function<void()>> ready_commands;
		std::mutex ready_commands_mutex;
	};

	export class BaseOpenGLRenderDevice : public BaseRenderDevice {
	protected:
		std::vector<FrameContext> m_frames;
		util::Subscriber* m_command_ready_subscription;	
		std::size_t m_next_frame_index;
		std::size_t m_previous_frame_index;
		
		std::atomic_flag m_allow_submission;
		bool m_is_minimized;
		bool m_is_window_alive;

		void OnCommandReady(CommandReadyEvent const& e);
		bool WaitForGPU(FrameContext& frame);
		bool WaitForNextFrame();
		bool WaitForPreviousFrame();

		virtual void SwapBuffer() = 0;

	public:
		template <std::convertible_to<core::LoggerPtr> Logger>
		BaseOpenGLRenderDevice(Logger&& logger, platform::IWindow& window, std::size_t frame_count)
			: BaseRenderDevice(std::forward<Logger>(logger), window),
			m_frames(frame_count),
			m_command_ready_subscription(
				m_message_bus.Subscribe<CommandReadyEvent>(
					[this](CommandReadyEvent const& e) {
						OnCommandReady(e);
					}
				)
			) {

		}

		BaseOpenGLRenderDevice(BaseOpenGLRenderDevice&& other) noexcept;
		BaseOpenGLRenderDevice& operator=(BaseOpenGLRenderDevice&& other) noexcept;

		virtual ~BaseOpenGLRenderDevice() noexcept override = default;

		virtual void Clear(float r, float g, float b, float a) override;
		virtual void SetViewport(int x, int y, std::uint32_t width, std::uint32_t height) override;

		virtual bool BeginFrame() override;
		virtual void EndFrame() override;

		API GetAPI() const noexcept override;

		virtual ICommandObject& AcquireCommandObject() override;

	};

	export template <class DerivedBuilder, class DerivedOpenGLRenderDevice>
		class BaseOpenGLRenderDeviceBuilder {
		protected:
			std::optional<DerivedOpenGLRenderDevice> m_render_device;
			core::LoggerPtr m_logger;
			platform::IWindow* m_window = nullptr;
			std::size_t m_frame_count;

		public:
			template <std::convertible_to<core::LoggerPtr> Logger>
			DerivedBuilder& SetLogger(Logger&& logger) noexcept {
				m_logger = util::MakeReferred(logger);
				return static_cast<DerivedBuilder&>(*this);
			}

			DerivedBuilder& SetWindow(platform::IWindow* window) noexcept {
				m_window = window;
				return static_cast<DerivedBuilder&>(*this);
			}

			DerivedBuilder& SetFrameCount(std::size_t frame_count) noexcept {
				m_frame_count = frame_count;
				return static_cast<DerivedBuilder&>(*this);
			}

			DerivedBuilder& Build() {

				/*
				*	Note:	The derived class must implement the BuildImp function whose signature is:
				*			void BuildImp().
				*			the m_render_device should be constructed in BuildImp function.
				*/

				static_cast<DerivedBuilder&>(*this).BuildImp();
				return static_cast<DerivedBuilder&>(*this);

			}

			std::optional<DerivedOpenGLRenderDevice> GetRenderDevice() noexcept {
				util::Defer defer(
					[this]() {
						m_render_device.reset();
					}
				);
				return std::move(m_render_device);
			}

	};

}