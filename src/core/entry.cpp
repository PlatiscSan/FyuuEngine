#ifdef WIN32
#include <Windows.h>
#endif // WIN32

import concurrent_vector;
import circular_buffer;
import pointer_wrapper;
import layer;
import event_system;
import simple_logger;
import config;
import imgui_layer;

static concurrency::ConcurrentVector<util::PointerWrapper<core::ILayer>> s_layers;
static concurrency::CircularBuffer<std::function<void()>, 32u> s_tasks;
static platform::IWindow* s_main_window;
static std::optional<logger::simple_logger::Logger> s_main_logger;
static std::optional<logger::simple_logger::FileSink> s_main_sink;
static bool s_quit = false;

static void OnMainWindowClosed(platform::WindowCloseEvent const& e) {
	if (s_main_window == e.source) {
		s_quit = true;
	}
}

static void OnMainWindowResized(platform::WindowResizeEvent const& e) {
	//if (s_main_window == e.source) {
	//	auto layers = s_layers.LockedModifier();
	//	for (auto& layer : layers) {
	//		layer->OnResize(e.width, e.height);
	//	}
	//}
}

static graphics::API GetAPIFromString(std::string const& api) {
	if (api == "d3d12") {
		return graphics::API::DirectX12;
	}
	else if (api == "vulkan") {
		return graphics::API::Vulkan;
	}
	else if (api == "opengl") {
		return graphics::API::OpenGL;
	}
	else {
		return graphics::API::Unknown;
	}
}

int main(int argc, char** argv) {

	/*
	*	load configuration from file;
	*/

	core::YAMLConfig config;
	core::ConfigNode& node = config.Node();
	if (!config.Open("./config.yaml")) {
#ifdef WIN32
		node.Set("engine.API", "d3d12");
#else
		node.Set("engine.API", "vulkan");
#endif // WIN32
		node.Set("engine.log_path", "./log/application.log");
		config.SaveAs("./config.yaml");
	}
	std::optional<std::string> api = node.Get<std::string>("engine.API");
	std::optional<std::string> log_path = node.Get<std::string>("engine.log_path");

	/*
	*	initialize logger
	*/

	s_main_sink.emplace(log_path.value_or("./log/application.log"));
	logger::simple_logger::LoggingCore::Instance()->RegisterSink("application", &s_main_sink.value());
	s_main_logger.emplace("application", true);


	/*
	*	initialize window
	*/

	util::Subscriber* close_sub;
	util::Subscriber* resize_sub;

#ifdef WIN32
	ImGui_ImplWin32_EnableDpiAwareness();
	float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

	std::uint32_t width = static_cast<std::uint32_t>(1280 * main_scale);
	std::uint32_t height = std::uint32_t(800 * main_scale);

#endif // WIN32

	try {
		platform::IWindow& main_window = platform::CreateMainWindow("FyuuEngine", width, height);
		s_main_window = &main_window;
		auto message_bus = main_window.GetMessageBus();
		close_sub = message_bus->Subscribe<platform::WindowCloseEvent>(OnMainWindowClosed);
		resize_sub = message_bus->Subscribe<platform::WindowResizeEvent>(OnMainWindowResized);

		graphics::BaseRenderDevice& renderer = graphics::CreateMainRenderDevice(
			&s_main_logger.value(),
			main_window,
			GetAPIFromString(api.value_or("unknown"))
		);

		ui::imgui::BaseIMGUILayer& imgui_layer = ui::imgui::CreateIMGUILayer(main_window, renderer);
		s_layers.emplace_back(&imgui_layer);

		main_window.Show();
		s_main_logger->Info(std::source_location::current(), "Application is now running");
		do {
			main_window.ProcessEvents();
			// Begin frame
			if (!renderer.BeginFrame()) {
				std::this_thread::yield();
				continue;
			}

			{
				auto layers = s_layers.LockedModifier();
				for (auto const& layer : layers) {
					layer->BeginFrame();
				}

				// Update
				for (auto& layer : layers) {
					layer->Update();
				}

				for (auto& layer : layers) {
					layer->Render();
				}
			}

			renderer.EndFrame();

			if (auto task = s_tasks.TryPopFront()) {
				(*task)();
			}
		} while (!s_quit);

		message_bus->Unsubscribe(resize_sub);
		message_bus->Unsubscribe(close_sub);

		return 0;
	}
#ifdef WIN32
	catch (platform::Win32Exception const& ex) {
		s_main_logger->Fatal(std::source_location::current(), "{}", ex.what());
		MessageBox(nullptr, ex.What(), TEXT("Fatal error"), MB_ICONERROR | MB_OK);
		return -1;
	}
#endif // WIN32
	catch (std::exception const& ex) {
		s_main_logger->Fatal(std::source_location::current(), "{}", ex.what());
		return -1;
	}

}