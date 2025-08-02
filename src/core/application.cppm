module;
#ifdef WIN32
#include <tchar.h>
#include <Windows.h>
#include <windowsx.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <comdef.h>
#include "d3dx12.h"
#endif // WIN32

export module application;
export import singleton;
import layer_interface;
import std;
import circular_buffer;
import simple_logger;
#ifdef WIN32
import d3d12_renderer;
#endif // WIN32


export namespace core {
	class Application : public util::Singleton<Application> {
	private:
		platform::IWindow* m_main_window;
		graphics::IRenderDevice* m_device;
		ILogger* m_main_logger;
		std::stack<util::PointerWrapper<ILayer>> m_layers;
		concurrency::CircularBuffer<std::function<void()>, 32u> m_tasks;
		bool quit = false;

		void OnWindowClosed(platform::WindowCloseEvent const& e) {
			if (e.source == m_main_window) {
				quit = true;
			}
		}

		void OnWindowResize(platform::WindowResizeEvent const& e) {
			if (e.source != m_main_window) {
				return;
			}
		}

	public:
		void Initialize() {

			// ----initialize logger----

			static simple_logger::FileSink main_sink("./log/application.log");
			simple_logger::LoggingCore::Instance()->RegisterSink("application", &main_sink);
			static simple_logger::Logger main_logger("application", true);
			m_main_logger = &main_logger;

			// ----initialize main window----

#ifdef WIN32
			static std::optional<platform::Win32Window> main_window;
			static std::optional<graphics::api::d3d12::D3D12RenderDevice> d3d12_device;
			try {
				main_window.emplace("Hello", 800, 600);
			}
			catch (platform::Win32Exception const& ex) {
				auto error_msg = platform::FromTChar(ex.what());
				m_main_logger->Fatal(std::source_location::current(), "Win32 exception thrown: {}", error_msg);
				MessageBox(nullptr, ex.what(), TEXT("Fatal error"), MB_ICONERROR | MB_OK);
				std::terminate();
			}
			catch (std::exception const& ex) {
				m_main_logger->Fatal(std::source_location::current(), "exception thrown: {}", ex.what());
				std::terminate();
			}

			m_main_window = &main_window.value();
			main_window->GetMessageBus()->Subscribe<platform::WindowCloseEvent>(
				[this](platform::WindowCloseEvent const& e) {
					OnWindowClosed(e);
				}
			);
			main_window->GetMessageBus()->Subscribe<platform::WindowResizeEvent>(
				[this](platform::WindowResizeEvent const& e) {
					OnWindowResize(e);
				}
			);

			try {
				d3d12_device.emplace(*main_window);
			}
			catch (_com_error const& ex) {
				auto error_msg = platform::FromTChar(ex.ErrorMessage());
				m_main_logger->Fatal(std::source_location::current(), "Win32 exception thrown: {}", error_msg);
				MessageBox(nullptr, ex.ErrorMessage(), TEXT("Fatal error"), MB_ICONERROR | MB_OK);
				std::terminate();
			}
			m_device = &d3d12_device.value();

#elif defined(__linux__)

#elif defined(__APPLE__)

#endif // WIN32

		}

		void Run() {
			m_main_logger->Info(std::source_location::current(), "Application is now running");
			m_main_window->Show();
			m_main_window->SetTitle("Hello World");
			do {
				m_main_window->ProcessEvents();
				if (auto task = m_tasks.TryPopFront()) {
					(*task)();
				}
			} while (!quit);
		}

	};
}