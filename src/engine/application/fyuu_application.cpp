#include <fyuu_application.h>

import adapter;
import std;

#ifdef WIN32
import windows_window;
import d3d12_backend;
import windows_vulkan;
import windows_opengl;
#endif // WIN32

import config.yaml;
import defer;

/*
*	helper macros to call internal implementation object
*/

#if defined(_WIN32) || defined(_WIN64)
#define INVOKE_RENDERER_FUNC(BACKEND, FUNC, OBJ, ...)\
	FYUU_DECLARE_BACKEND_SWITCH(\
		BACKEND, \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::windows::vulkan::WindowsVulkanRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::windows::opengl::WindowsOpenGLRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::windows::d3d12::D3D12Renderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		; \
	)\

#define RETURN_RENDERER_FUNC(BACKEND, FUNC, OBJ, ...)\
	FYUU_DECLARE_BACKEND_RETURN_SWITCH(\
		BACKEND, \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::windows::vulkan::WindowsVulkanRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::windows::opengl::WindowsOpenGLRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::windows::d3d12::D3D12Renderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		{}; \
	)\

#define RETURN_PTR_RENDERER_FUNC(BACKEND, FUNC, OBJ, ...)\
	FYUU_DECLARE_BACKEND_RETURN_SWITCH(\
		BACKEND, \
		&FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::windows::vulkan::WindowsVulkanRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		&FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::windows::opengl::WindowsOpenGLRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		&FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::windows::d3d12::D3D12Renderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		nullptr; \
	)\

#elif defined(__linux__)
#define INVOKE_RENDERER_FUNC(BACKEND, FUNC, OBJ, ...)\
	FYUU_DECLARE_BACKEND_SWITCH(\
		BACKEND, \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::linux::vulkan::LinuxVulkanRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::linux::opengl::LinuxOpenGLRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		; \
	)\

#define RETURN_RENDERER_FUNC(BACKEND, FUNC, OBJ, ...)\
	FYUU_DECLARE_BACKEND_RETURN_SWITCH(\
		BACKEND, \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::linux::vulkan::LinuxVulkanRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::linux::opengl::LinuxOpenGLRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		{}; \
	)\

#define RETURN_PTR_RENDERER_FUNC(BACKEND, FUNC, OBJ, ...)\
	FYUU_DECLARE_BACKEND_RETURN_SWITCH(\
		BACKEND, \
		&FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::linux::vulkan::LinuxVulkanRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		&FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::linux::opengl::LinuxOpenGLRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		nullptr; \
	)\

#elif defined(__APPLE__) && defined(__MACH__)
#define INVOKE_RENDERER_FUNC(BACKEND, FUNC, OBJ, ...)\
	FYUU_DECLARE_BACKEND_SWITCH(\
		BACKEND, \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::apple::vulkan::AppleVulkanRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::apple::opengl::AppleOpenGLRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::apple::metal::MetalRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		; \
	)\

#define RETURN_RENDERER_FUNC(BACKEND, FUNC, OBJ, ...)\
	FYUU_DECLARE_BACKEND_RETURN_SWITCH(\
		BACKEND, \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::apple::vulkan::AppleVulkanRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::apple::opengl::AppleOpenGLRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::apple::metal::MetalRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		{}; \
	)\

#define RETURN_PTR_RENDERER_FUNC(BACKEND, FUNC, OBJ, ...)\
	FYUU_DECLARE_BACKEND_RETURN_SWITCH(\
		BACKEND, \
		&FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::apple::vulkan::AppleVulkanRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		&FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::apple::opengl::AppleOpenGLRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		&FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::apple::metal::MetalRenderer, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		nullptr; \
	)\

#endif

namespace fyuu_engine::application {

	static std::atomic_flag s_app_mutex;
	static bool s_app_running;

	using RendererInterface = Renderer;
	using RenderingThreads = std::array<std::optional<concurrency::Worker>, 20u>;
	
	/*
	*	TODO: add json config support
	*/

	using Config = std::variant<
		std::monostate,
		config::YAMLConfig
	>;

	static ILogger* InitDefaultLogger(
		std::optional<logger::simple_logger::FileSink>& opt_main_sink,
		bool write_to_file,
		std::size_t max_size,
		std::optional<logger::simple_logger::Logger>& opt_logger,
		std::optional<SimpleLoggerAdapter>& opt_adapter
	) {

		using namespace logger::simple_logger;

		if (write_to_file) {

			auto& main_sink = opt_main_sink.emplace("./logs/application.log", max_size);
			LoggingCore::Instance()->RegisterSink("main_sink", &main_sink);
			auto& logger = opt_logger.emplace("main_sink");
			auto& adapter = opt_adapter.emplace(&logger);

			return &adapter;

		}
		else {

			auto& logger = opt_logger.emplace("");
			auto& adapter = opt_adapter.emplace(&logger);

			return &adapter;

		}

	}

	static std::size_t LaunchRenderingThreads(std::int32_t thread_count, RenderingThreads& threads) {

		std::size_t launch_count = (std::min)(
			thread_count < 0 ? static_cast<std::size_t>(std::thread::hardware_concurrency()) : static_cast<std::size_t>(thread_count),
			threads.size()
			);

		for (std::size_t i = 0; i < launch_count; ++i) {
			threads[i].emplace();
		}

		return launch_count;

	}

	template <class Window>
		requires requires(Window window) {
		window.MessageBus();
	}
	std::vector<std::shared_ptr<concurrency::ISubscription>> SubscribeEvent(
		Window&& window,
		IApplication* app
	) {

		std::vector<std::shared_ptr<concurrency::ISubscription>> subscriptions;

		concurrency::SyncMessageBus& message_bus = window.MessageBus();
		subscriptions.emplace_back(
			message_bus.Subscribe<windows::WindowCloseEvent>(
				[app]() {
					app->OnClose();
				}
			)
		);

		subscriptions.emplace_back(
			message_bus.Subscribe<windows::WindowResizeEvent>(
				[app](windows::WindowResizeEvent const& e) {
					app->OnResize(e.GetWidth(), e.GetHeight());
				}
			)
		);

		subscriptions.emplace_back(
			message_bus.Subscribe<windows::KeyboardEvent>(
				[app](windows::KeyboardEvent const& e) {
					switch (e.GetAction()) {
					case windows::KeyboardEvent::Action::Press:
						app->OnKeyDown(e.GetKeyCode());
						break;

					case windows::KeyboardEvent::Action::Release:
						app->OnKeyUp(e.GetKeyCode());
						break;

					case windows::KeyboardEvent::Action::Repeat:
						app->OnKeyRepeat(e.GetKeyCode());
						break;
					}
				}
			)
		);

		subscriptions.emplace_back(
			message_bus.Subscribe<windows::MouseMoveEvent>(
				[app](windows::MouseMoveEvent const& e) {
					app->OnMouseMove(e.GetX(), e.GetY());
				}
			)
		);

		subscriptions.emplace_back(
			message_bus.Subscribe<windows::MouseButtonEvent>(
				[app](windows::MouseButtonEvent const& e) {
					switch (e.GetAction()) {
					case windows::MouseButtonEvent::Action::Press:
						app->OnMouseButtonDown(static_cast<Button>(e.GetButton()), e.GetX(), e.GetY());
						break;

					case windows::MouseButtonEvent::Action::Release:
						app->OnMouseButtonUp(static_cast<Button>(e.GetButton()), e.GetX(), e.GetY());
						break;
					}
				}
			)
		);

		return subscriptions;

	}

	template <class Window, class Renderer>
	RendererInterface CreateRenderer(
		Config const& backend_config, 
		core::IRendererLogger* logger, 
		Window&& main_window, 
		Renderer&& renderer
	) {

		/*
		*	load renderer configuration
		*/

		API backend = API::Vulkan;
		std::string_view app_name;

		std::visit(
			[&backend, &app_name](auto&& config) {

				using T = std::decay_t<decltype(config)>;
				if constexpr (std::is_same_v<T, config::YAMLConfig>) {
					backend = static_cast<API>(config["graphics"]["backend"].GetOr<std::uint32_t>(1u));
					app_name = config["engine"]["app_name"].Get<std::string>();
				}
				else {
					throw std::runtime_error("unsupported config format for renderer");
				}

			},
			backend_config
		);

#ifdef WIN32
		std::variant<
			std::monostate,
			windows::d3d12::D3D12RendererBuilder,
			windows::vulkan::WindowsVulkanRendererBuilder,
			windows::opengl::WindowsOpenGLRendererBuilder
		> builder_variant;

		switch (backend) {
		case API::DirectX12:
		{
			auto& impl = renderer.emplace<std::optional<windows::d3d12::D3D12Renderer>>();
			builder_variant.emplace<windows::d3d12::D3D12RendererBuilder>()
				.SetLogger(logger)
				.SetFrameCount(3)
				.SetTargetWindow(&main_window)
				.Build(impl);
			return RendererInterface(&impl, API::DirectX12);
		}

		case API::Vulkan:
		{
			auto& impl = renderer.emplace<std::optional<windows::vulkan::WindowsVulkanRenderer>>();
			builder_variant.emplace<windows::vulkan::WindowsVulkanRendererBuilder>()
				.SetTargetWindow(&main_window)
				.SetLogger(logger)
				.SetEngineName("Fyuu Engine")
				.SetApplicationName(app_name)
				.Build(impl);
			return RendererInterface(&impl, API::Vulkan);
		}

		case API::OpenGL:
		{
			auto& impl = renderer.emplace<std::optional<windows::opengl::WindowsOpenGLRenderer>>();
			builder_variant.emplace<windows::opengl::WindowsOpenGLRendererBuilder>()
				.SetTargetWindow(&main_window)
				.SetLogger(logger)
				.Build(impl);
			return RendererInterface(&impl, API::OpenGL);
		}

		default:
			throw std::runtime_error("unsupported graphics backend");
		}
#endif // WIN32


	}

	static bool BeginFrame(RendererInterface& renderer) {
		RETURN_RENDERER_FUNC(renderer.GetBackend(), BeginFrame, renderer.GetImplementation());
	}

	static void EndFrame(RendererInterface& renderer) {
		INVOKE_RENDERER_FUNC(renderer.GetBackend(), EndFrame, renderer.GetImplementation());
	}

#ifdef _WIN32
	using OutputTarget = std::variant<
		std::monostate,
		vulkan::VulkanFrameBuffer,
		opengl::OpenGLFrameBuffer,
		windows::d3d12::D3D12RenderTarget
	>;
#else
	using OutputTarget = std::variant<
		std::monostate,
		vulkan::VulkanFrameBuffer,
		opengl::OpenGLFrameBuffer,
	>;
#endif // _WIN32


	static OutputTarget OutputTargetOfCurrentFrame(RendererInterface& renderer) {
		RETURN_RENDERER_FUNC(renderer.GetBackend(), OutputTargetOfCurrentFrame, renderer.GetImplementation());
	}

	static void* GetCommandObject(RendererInterface& renderer) {
		RETURN_PTR_RENDERER_FUNC(renderer.GetBackend(), GetCommandObject, renderer.GetImplementation());
	}

	template <class Window>
		requires requires(Window window) {
		window.Show();
		window.ProcessEvents();
	}
	static void MainLoop(
		Window&& window,
		RendererInterface& renderer,
		RenderingThreads& threads, 
		std::size_t threads_count,
		IApplication* app
	) {

		window.Show();
		s_app_running = true;

		while (s_app_running) {
			window.ProcessEvents();

			if (!BeginFrame(renderer)) {
				std::this_thread::yield();
				continue;
			}

			std::latch latch(threads_count + 1);
			OutputTarget output_target = OutputTargetOfCurrentFrame(renderer);
			void* output_target_impl_ptr = std::visit(
				[](auto&& impl) -> void* {
					if constexpr (std::is_same_v<std::monostate, std::decay_t<decltype(impl)>>) {
						return nullptr;
					}
					else {
						return &impl;
					}
				},
				output_target
			);

			auto Tick = [app, &renderer, output_target_impl_ptr]() {
				CommandObject command_object(GetCommandObject(renderer), output_target_impl_ptr, renderer.GetBackend());
				auto id = std::this_thread::get_id();
				app->OnUpdate(id, renderer);
				app->OnRender(id, command_object);
				};

			for (std::size_t i = 0; i < threads_count; ++i) {
				threads[i]->SubmitTask(
					[&latch, &Tick]() {
						Tick();
						latch.count_down();
					}
				);
			}

			Tick();
			latch.arrive_and_wait();

			EndFrame(renderer);
		}

	}

	/// @brief Dynamic libary entry point
	int DLL_CALL RunApp(IApplication* app) NO_EXCEPT try {

		if (s_app_mutex.test_and_set(std::memory_order::acq_rel)) {
			return -1;
		}
		util::Defer gc(
			[]() {
				s_app_mutex.clear(std::memory_order::release);
			}
		);
		
		std::optional<logger::simple_logger::FileSink> simple_logger_sink;
		std::optional<logger::simple_logger::Logger> simple_logger;
		std::optional<SimpleLoggerAdapter> adapter;
		std::array<std::optional<concurrency::Worker>, 20u> threads;
		ApplicationConfig config = app->GetConfig();

		std::string_view app_name;
		std::uint32_t width;
		std::uint32_t height;
		LogLevel log_level;
		bool write_to_file;
		std::size_t max_log_size;
		std::int32_t thread_count;

		/*
		*	load configuration
		*/

		Config backend_config;

		switch (config.format) {
		case ApplicationConfig::ConfigFormat::YAML:
		{
			config::YAMLConfig& yaml_config = backend_config.emplace<config::YAMLConfig>();
			yaml_config.Open(config.path);
			app_name = yaml_config["engine"]["app_name"].Get<std::string>();
			width = yaml_config["graphics"]["resolution"]["width"].Get<std::uint32_t>();
			height = yaml_config["graphics"]["resolution"]["height"].Get<std::uint32_t>();
			log_level = static_cast<LogLevel>(yaml_config["logging"]["level"].Get<std::uint32_t>());
			write_to_file = yaml_config["logging"]["write_to_file"].Get<bool>();
			max_log_size = yaml_config["logging"]["max_file_size"].Get<std::size_t>();
			thread_count = yaml_config["performance"]["thread_count"].Get<std::int32_t>();
			break;
		}
		default:
			break;
		}

		/*
		*	initialize logger
		*/

		ILogger* custom_logger = app->CustomLogger(log_level, write_to_file, max_log_size);
		ILogger* logger = 
			custom_logger ? 
			custom_logger : 
			InitDefaultLogger(
				simple_logger_sink,
				write_to_file,
				max_log_size, 
				simple_logger, 
				adapter
			);

		/*
		*	create window
		*/

#ifdef _WIN32
		windows::WindowsWindow main_window(app_name, width, height);
		auto event_subscriptions = SubscribeEvent(main_window, app);
#endif // _WIN32

		/*
		*	create renderer and launch rendering threads
		*/

		RendererLoggerAdapter renderer_logger(logger);

#ifdef _WIN32
		std::variant<
			std::monostate,
			std::optional<windows::d3d12::D3D12Renderer>,
			std::optional<windows::vulkan::WindowsVulkanRenderer>,
			std::optional<windows::opengl::WindowsOpenGLRenderer>
		> renderer;
#endif // _WIN32
		RendererInterface renderer_interface = CreateRenderer(
			backend_config, 
			&renderer_logger,
			main_window, 
			renderer
		);
		std::size_t threads_count = LaunchRenderingThreads(
			thread_count,
			threads
		);

		logger->Info(std::source_location::current(), "Application is running");

		MainLoop(main_window, renderer_interface, threads, threads_count, app);

		return 0;

	}
	catch (std::exception const& ex) {
		std::cerr << "failed to run application: " << ex.what() << std::endl;
		return -1;
	}
	catch (...) {
		std::cerr << "unknown error occurred" << std::endl;
		return -1;
	}

	void DLL_CALL RequestStop() NO_EXCEPT {
		s_app_running = false;
	}

	ApplicationConfig::ApplicationConfig(Fyuu_ApplicationConfig const& c_config)
		: path(c_config.path ? std::filesystem::path(c_config.path) : std::filesystem::path()) {
	}

	ILogger* IApplication::CustomLogger(LogLevel level, bool write_to_file, std::size_t max_size) {
		return nullptr;
	}

	ApplicationConfig IApplication::GetConfig() const {

		ApplicationConfig config;
		config.path = "./config/app_config.yaml";

		return config;
	}

	void IApplication::OnClose() {
		RequestStop();
	}

	void IApplication::OnResize(std::uint32_t width, std::uint32_t height) {

	}

	void IApplication::OnKeyDown(int key) {

	}

	void IApplication::OnKeyUp(int key) {

	}

	void IApplication::OnKeyRepeat(int key) {

	}

	void IApplication::OnMouseMove(double x, double y) {

	}

	void IApplication::OnMouseButtonDown(Button button, double x, double y) {

	}

	void IApplication::OnMouseButtonUp(Button button, double x, double y) {

	}

	void IApplication::OnUpdate(std::thread::id const& id, Renderer& renderer) {
		
	}

	void IApplication::OnRender(std::thread::id const& id, CommandObject& command_object) {

	}

}

int DLL_CALL Fyuu_RunApp(Fyuu_IApplication* app) NO_EXCEPT {

	fyuu_engine::application::CApplicationAdapter app_adapter(app);

	return fyuu_engine::application::RunApp(&app_adapter);

}

void DLL_CALL Fyuu_RequestStop() NO_EXCEPT {
	fyuu_engine::application::RequestStop();
}