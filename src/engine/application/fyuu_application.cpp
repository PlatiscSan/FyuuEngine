#include <fyuu_application.h>

import adapter;
import std;

#ifdef WIN32
import windows_window;
import d3d12_backend;
import windows_vulkan;
import windows_opengl;
#endif // WIN32

namespace fyuu_engine::application {

	/*
	*	helper macros to call implementation object
	*/
#ifdef _WIN32
	#define CALL_BACKEND_FUNC(FUNC_NAME, BACKEND, IMPL, ...) \
	switch (static_cast<API>(BACKEND)) { \
		case API::Vulkan: \
			static_cast<windows::vulkan::WindowsVulkanRenderer*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
			break;\
		case API::OpenGL:\
			static_cast<windows::opengl::WindowsOpenGLRenderer*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
			break;\
		case API::DirectX12: \
			static_cast<windows::d3d12::D3D12Renderer*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
			break;\
		default: \
			break; \
	}
	#define RETURN_BACKEND_FUNC(FUNC_NAME, DEFAULT_RETURN_STATEMENT, BACKEND, IMPL, ...) \
	switch (static_cast<API>(BACKEND)) { \
		case API::Vulkan: \
			return static_cast<windows::vulkan::WindowsVulkanRenderer*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
		case API::OpenGL:\
			return static_cast<windows::opengl::WindowsOpenGLRenderer*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
		case API::DirectX12: \
			return static_cast<windows::d3d12::D3D12Renderer*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
		default: \
			DEFAULT_RETURN_STATEMENT; \
	}
	#define RETURN_BACKEND_FUNC_PTR(FUNC_NAME, DEFAULT_RETURN_STATEMENT, BACKEND, IMPL, ...) \
	switch (static_cast<API>(BACKEND)) { \
		case API::Vulkan: \
			return &static_cast<windows::vulkan::WindowsVulkanRenderer*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
		case API::OpenGL:\
			return &static_cast<windows::opengl::WindowsOpenGLRenderer*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
		case API::DirectX12: \
			return &static_cast<windows::d3d12::D3D12Renderer*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
		default: \
			DEFAULT_RETURN_STATEMENT; \
	}
#else
	/*
	*	TODO: other platform, can duplicate the definition above
	*/

#endif // _WIN32

	static std::atomic_flag s_app_mutex;
	static bool s_app_running;

	using RendererInterface = Renderer;
	using RenderingThreads = std::array<std::optional<concurrency::Worker>, 20u>;

	static ILogger* InitDefaultLogger(
		std::optional<logger::simple_logger::FileSink>& opt_main_sink,
		std::optional<logger::simple_logger::Logger>& opt_logger,
		std::optional<SimpleLoggerAdapter>& opt_adapter
	) {

		using namespace logger::simple_logger;

		auto& main_sink = opt_main_sink.emplace("/logs/application.log");
		LoggingCore::Instance()->RegisterSink("main_sink", &main_sink);

		auto& logger = opt_logger.emplace("main_sink");
		auto& adapter = opt_adapter.emplace(&logger);

		return &adapter;

	}

	static std::size_t LaunchRenderingThreads(ApplicationConfig const& config, RenderingThreads& threads) {

		std::size_t launch_count = (std::min)(config.rendering_threads, threads.size());

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
		ApplicationConfig const& config, 
		core::IRendererLogger* logger, 
		Window&& main_window, 
		Renderer&& renderer
	) {

#ifdef WIN32
		std::variant<
			std::monostate,
			windows::d3d12::D3D12RendererBuilder,
			windows::vulkan::WindowsVulkanRendererBuilder,
			windows::opengl::WindowsOpenGLRendererBuilder
		> builder_variant;

		switch (config.backend) {
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
				.SetApplicationName(config.application_name)
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
		RETURN_BACKEND_FUNC(BeginFrame, return false, renderer.GetBackend(), renderer.GetImplementation());
	}

	static void EndFrame(RendererInterface& renderer) {
		CALL_BACKEND_FUNC(EndFrame, renderer.GetBackend(), renderer.GetImplementation());
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
		RETURN_BACKEND_FUNC(OutputTargetOfCurrentFrame, return std::monostate{}, renderer.GetBackend(), renderer.GetImplementation());
	}

	static void* GetCommandObject(RendererInterface& renderer) {
		RETURN_BACKEND_FUNC_PTR(GetCommandObject, return nullptr, renderer.GetBackend(), renderer.GetImplementation());
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

			auto Render = [app, &renderer, output_target_impl_ptr]() {
				CommandObject command_object(GetCommandObject(renderer), output_target_impl_ptr, renderer.GetBackend());
				app->OnUpdate(std::this_thread::get_id(), renderer);
				app->OnRender(std::this_thread::get_id(), command_object);
				};

			for (std::size_t i = 0; i < threads_count; ++i) {
				threads[i]->SubmitTask(
					[&latch, &Render]() {
						Render();
						latch.count_down();
					}
				);
			}

			Render();
			latch.arrive_and_wait();

			EndFrame(renderer);
		}

	}

	/// @brief Dynamic libary entry point
	int DLL_CALL RunApp(IApplication* app) NO_EXCEPT try {

		if (s_app_mutex.test_and_set(std::memory_order::acq_rel)) {
			return -1;
		}
		
		std::optional<logger::simple_logger::FileSink> simple_logger_sink;
		std::optional<logger::simple_logger::Logger> simple_logger;
		std::optional<SimpleLoggerAdapter> adapter;
		std::array<std::optional<concurrency::Worker>, 20u> threads;
		static std::size_t rendering_threads_count;
		ApplicationConfig config = app->GetConfig();

		/*
		*	initialize logger
		*/

		ILogger* custom_logger = app->CustomLogger();
		ILogger* logger = custom_logger ? custom_logger : InitDefaultLogger(simple_logger_sink, simple_logger, adapter);

		/*
		*	create window
		*/

#ifdef _WIN32
		windows::WindowsWindow main_window(config.title, config.width, config.height);
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
		RendererInterface renderer_interface = CreateRenderer(config, &renderer_logger, main_window, renderer);
		std::size_t threads_count = LaunchRenderingThreads(config, threads);

		logger->Info(std::source_location::current(), "Application is running");

		MainLoop(main_window, renderer_interface, threads, threads_count, app);

		s_app_mutex.clear(std::memory_order::release);

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

	ApplicationConfig::ApplicationConfig(Fyuu_ApplicationConfig const& config) noexcept
		: application_name(config.application_name),
		title(config.title),
		width(config.width),
		height(config.height),
		rendering_threads(config.rendering_threads),
		backend(static_cast<API>(config.backend)) {

	}

	ILogger* IApplication::CustomLogger() {
		return nullptr;
	}

	ApplicationConfig IApplication::GetConfig() const {

		ApplicationConfig config;

		config.application_name = "Fyuu Engine";
		config.title = "Fyuu Engine";
		config.width = 1280u;
		config.height = 720u;
		config.rendering_threads = 4u;
		config.backend = API::Vulkan;

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