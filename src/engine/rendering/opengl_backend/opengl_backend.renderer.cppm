module;
#include <glad/glad.h>

export module opengl_backend:renderer;
export import rendering;
export import :command_object;
import concurrent_hash_map;
import defer;

namespace fyuu_engine::opengl {

	void LoggerCallback(
		GLenum source, GLenum type, unsigned int id, GLenum severity,
		GLsizei length, char const* message, void const* user_param
	);

	GLenum GetBufferTarget(core::BufferType type) noexcept;
	GLenum GetBufferUsage(core::BufferType type) noexcept;

	export struct OpenGLRendererTraits {
		using CommandObjectType = OpenGLCommandObject;
		using CollectiveBufferType = UniqueOpenGLBuffer;
		using OutputTargetType = OpenGLFrameBuffer;
	};

	export template <class DerivedOpenGLRenderer> class BaseOpenGLRenderer
		: public core::BaseRenderer<DerivedOpenGLRenderer, OpenGLRendererTraits> {
		friend class core::BaseRenderer<DerivedOpenGLRenderer, OpenGLRendererTraits>;
	protected:
		using ThreadCommandObjects = concurrency::ConcurrentHashMap<std::thread::id, OpenGLCommandObject>;
		using InstanceCommandObjects = concurrency::ConcurrentHashMap<DerivedOpenGLRenderer*, ThreadCommandObjects>;
		static inline InstanceCommandObjects s_command_objects;

		OpenGLFrameBuffer m_frame_buffer;
		std::shared_ptr<concurrency::ISubscription> m_command_ready_subscription;

		std::vector<CommandBuffer> m_ready_commands;
		std::mutex m_ready_commands_mutex;

		std::atomic_flag m_allow_submission;
		bool m_is_minimized = false;
		bool m_is_window_alive = true;

		void OnCommandReady(OpenGLCommandBufferReady e) {

			while (!m_allow_submission.test(std::memory_order::acquire)) {
				m_allow_submission.wait(false, std::memory_order::relaxed);
			}

			std::lock_guard<std::mutex> lock(m_ready_commands_mutex);
			m_ready_commands.emplace_back(std::move(e.buffer));

		}

		OpenGLCommandObject& GetCommandObjectImpl() {

			auto thread_command_objects = s_command_objects.find(static_cast<DerivedOpenGLRenderer*>(this));
			if (!thread_command_objects) {
				throw std::runtime_error("no renderer registration");
			}

			auto [command_object_ptr, success] = thread_command_objects->try_emplace(
				std::this_thread::get_id(),
				&(static_cast<DerivedOpenGLRenderer*>(this))->m_message_bus
			);

			static thread_local util::Defer gc(
				[]() {
					auto modifier = s_command_objects.LockedModifier();
					for (auto& [instance, thread_command_objects] : modifier) {
						thread_command_objects.erase(std::this_thread::get_id());
					}
				}
			);

			return *command_object_ptr;

		}

		bool BeginFrameImpl() {

			bool should_proceed =
				!m_is_minimized &&
				m_is_window_alive;

			if (!should_proceed) {
				return false;
			}

			m_allow_submission.test_and_set(std::memory_order::release);
			m_allow_submission.notify_all();

			return true;

		}

		void EndFrameImpl() {

			/*
			*	No more submission to this frame
			*/

			m_allow_submission.clear(std::memory_order::release);

			static_cast<DerivedOpenGLRenderer*>(this)->MakeCurrent();

			for (auto& ready_commands : m_ready_commands) {
				while (!ready_commands.empty()) {
					ready_commands.front()();
					ready_commands.pop_front();
				}
			}

			glFlush();
			static_cast<DerivedOpenGLRenderer*>(this)->SwapBuffer();

		}

		void CreateFrameBuffer(std::uint32_t width, std::uint32_t height) {

			static_cast<DerivedOpenGLRenderer*>(this)->MakeCurrent();

			glDeleteTextures(1, &m_frame_buffer.color_texture);
			glDeleteTextures(1, &m_frame_buffer.depth_texture);

			GLuint color_texture;
			GLuint depth_texture;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glGenTextures(1, &color_texture);
			glBindTexture(GL_TEXTURE_2D, color_texture);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
				GL_RGBA, GL_UNSIGNED_BYTE, nullptr
			);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_2D, color_texture, 0
			);


			glGenTextures(1, &depth_texture);
			glBindTexture(GL_TEXTURE_2D, depth_texture);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0,
				GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr
			);
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
				GL_TEXTURE_2D, depth_texture, 0
			);

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

			if (status != GL_FRAMEBUFFER_COMPLETE) {
				throw std::runtime_error("Framebuffer not complete!");
			}

			m_frame_buffer.color_texture = color_texture;
			m_frame_buffer.depth_texture = depth_texture;
			m_frame_buffer.width = width;
			m_frame_buffer.height = height;
		}

		OpenGLFrameBuffer OutputTargetOfCurrentFrameImpl() {
			return m_frame_buffer;
		}

		UniqueOpenGLBuffer CreateBufferImpl(core::BufferDescriptor const& desc) {
			OpenGLCommandObject& command_object = this->GetCommandObject();
			return UniqueOpenGLBuffer(
				OpenGLBuffer(
					&command_object, 
					GetBufferTarget(desc.buffer_type), 
					GetBufferUsage(desc.buffer_type), 
					desc.size
				),
				OpenGLBufferCollector()
			);
		}

	public:
		using Base = core::BaseRenderer<DerivedOpenGLRenderer, OpenGLRendererTraits>;

		BaseOpenGLRenderer(core::LoggerPtr const& logger)
			: Base(logger),
			m_frame_buffer(),
			m_command_ready_subscription(
				this->m_message_bus.Subscribe<OpenGLCommandBufferReady>(
					[this](OpenGLCommandBufferReady e) {
						OnCommandReady(e);
					}
				)
			) {
			(void)s_command_objects.try_emplace(static_cast<DerivedOpenGLRenderer*>(this), ThreadCommandObjects());
		}

		~BaseOpenGLRenderer() noexcept {
			glDeleteTextures(1, &m_frame_buffer.color_texture);
			glDeleteTextures(1, &m_frame_buffer.depth_texture);
		}

	};

	export template <class DerivedOpenGLRendererBuilder, class DerivedOpenGLRenderer> class BaseOpenGLRendererBuilder 
		: public core::BaseRendererBuilder<BaseOpenGLRendererBuilder<DerivedOpenGLRendererBuilder, DerivedOpenGLRenderer>, DerivedOpenGLRenderer> {
		friend class core::BaseRendererBuilder<BaseOpenGLRendererBuilder<DerivedOpenGLRendererBuilder, DerivedOpenGLRenderer>, DerivedOpenGLRenderer>;
	private:
		static bool IsDebugContextEnabled() noexcept {
			GLint flags;
			glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
			return (flags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0;
		}

		static std::vector<std::string_view> CheckExtensionsSupport(std::span<char const*> extension_names) {

			std::unordered_set<std::string> supported_extension_names;

			GLint extensions_count = 0;
			glGetIntegerv(GL_NUM_EXTENSIONS, &extensions_count);

			for (GLint i = 0; i < extensions_count; ++i) {
				supported_extension_names.emplace(reinterpret_cast<char const*>(glGetStringi(GL_EXTENSIONS, i)));
			}

			std::vector<std::string_view> unsupported_extension_names;

			for (auto const& extension_name : extension_names) {
				auto iter = supported_extension_names.find(extension_name);
				if (iter == supported_extension_names.end()) {
					unsupported_extension_names.emplace_back(extension_name);
				}
			}

			return unsupported_extension_names;

		}

		std::shared_ptr<DerivedOpenGLRendererBuilder> BuildImpl() {
			return static_cast<DerivedOpenGLRendererBuilder*>(this)->BuildImpl();
		}

		void BuildImpl(std::optional<DerivedOpenGLRenderer>& buffer) {
			static_cast<DerivedOpenGLRendererBuilder*>(this)->BuildImpl(buffer);
		}

		void BuildImpl(std::span<std::byte> buffer) {
			if (buffer.size() < sizeof(DerivedOpenGLRendererBuilder)) {
				return;
			}
			static_cast<DerivedOpenGLRendererBuilder*>(this)->BuildImpl(buffer);
		}

	protected:
		static inline bool s_debug_context_enabled = true;
		static inline bool s_spirv_supported = true;

		void BindLogger() {

			if (GLAD_GL_KHR_debug) {
				this->m_logger->Log(core::LogLevel::Info, "KHR_debug extension is supported!");
			}
			else if (GLAD_GL_ARB_debug_output) {
				this->m_logger->Log(core::LogLevel::Info, "ARB_debug_output extension is supported!");
			}
			else {
				this->m_logger->Log(core::LogLevel::Warning, "Debug output not supported!");
				return;
			}

			glDebugMessageCallback(LoggerCallback, this->m_logger.Get());

		}

		void InitializeOpenGL() {
			static std::once_flag initialized;
			std::call_once(
				initialized, 
				[this]() {				
					static_cast<DerivedOpenGLRendererBuilder*>(this)->InitializeOpenGLImpl();
				}
			);
		}

		void CheckSupports() {

			s_debug_context_enabled = IsDebugContextEnabled();
			if (s_debug_context_enabled) {
				glEnable(GL_DEBUG_OUTPUT);
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			}

			std::array extension_names = {
				"GL_ARB_gl_spirv"
			};

			std::vector<std::string_view> unsupported_extension_names = CheckExtensionsSupport(
				{ extension_names.begin(), extension_names.end() }
			);

			for (auto const& unsupported_extension_name : unsupported_extension_names) {
				if (unsupported_extension_name == "GL_ARB_gl_spirv") {
					s_spirv_supported = false;
				}
			}

		}

	public:
		static bool IsSPIRVSupported() noexcept {
			return s_spirv_supported;
		}
	};

}
