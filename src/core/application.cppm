module;
#ifdef WIN32
#include <Windows.h>
#endif // WIN32

export module application;
export import singleton;
import std;
import circular_buffer;
import simple_logger;
#ifdef WIN32
import windows_window;
#endif // WIN32


export namespace core {
	class Application : public util::Singleton<Application> {
	private:
		IWindow* m_main_window;
		logger::ILogger* m_main_logger;
		concurrency::CircularBuffer<std::function<void()>, 32u> m_tasks;
		bool quit = false;

		void OnWindowClosed(typename IWindow::WindowCloseEvent const& e) {
			if (e.source == m_main_window) {
				quit = true;
			}
		}

		void OnWindowResize(typename IWindow::WindowResizeEvent const& e) {
			if (e.source != m_main_window) {
				return;
			}
		}

	public:
		void Initialize() {

			// ----initialize logger----

			static logger::simple_logger::FileSink main_sink("./log/application.log");
			logger::simple_logger::LoggingCore::Instance()->RegisterSink("application", &main_sink);
			static logger::simple_logger::Logger main_logger("application", true);
			m_main_logger = &main_logger;

			// ----initialize main window----

#ifdef WIN32
			static WindowsWindow main_window;
			try {
				main_window.Create("Hello", 800, 600);
			}
			catch (Win32Exception const& ex) {
				m_main_logger->Fatal(std::source_location::current(), "Win32 exception thrown: {}", ex.what());
				MessageBox(nullptr, ex.ErrorMessage(), TEXT("Fatal error"), MB_ICONERROR | MB_OK);
				std::terminate();
			}
			catch (std::exception const& ex) {
				m_main_logger->Fatal(std::source_location::current(), "exception thrown: {}", ex.what());
				std::terminate();
			}

			m_main_window = &main_window;
			main_window.GetMessageBus()->Subscribe<typename IWindow::WindowCloseEvent>(
				[this](typename IWindow::WindowCloseEvent const& e) {
					OnWindowClosed(e);
				}
			);
			main_window.GetMessageBus()->Subscribe<typename IWindow::WindowResizeEvent>(
				[this](typename IWindow::WindowResizeEvent const& e) {
					OnWindowResize(e);
				}
			);

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