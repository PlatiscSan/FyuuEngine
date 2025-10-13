module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef WIN32
#include <tchar.h>
#include <Windows.h>
#endif // WIN32

export module windows_window;
export import window;
import std;
import concurrent_vector;
import windows_utils;

namespace fyuu_engine::windows {
#ifdef WIN32

	export class WindowsWindow :
		public core::BaseWindow<WindowsWindow> {
	public:
		friend class core::BaseWindow<WindowsWindow>;
		using MsgProcessor = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>;
		using ClassName = std::array<TCHAR, 37u>;

	private:
		concurrency::ConcurrentVector<std::pair<boost::uuids::uuid, MsgProcessor>> m_msg_processors;
		ClassName m_class_name;
		HWND m_hwnd = nullptr;

		static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept;

		static ClassName GenerateClassName();

		static HWND CreateWindowImpl(
			WindowsWindow* window,
			std::string_view title,
			std::uint32_t width,
			std::uint32_t height
		);

		void OnDestroy() noexcept;
		void OnResize(LPARAM lparam) noexcept;
		void OnKeyDown(WPARAM wparam, LPARAM lparam) noexcept;
		void OnKeyUp(WPARAM wparam, LPARAM lparam) noexcept;
		void OnMouseButtonDown(UINT msg, LPARAM lparam) noexcept;
		void OnMouseButtonUp(UINT msg, LPARAM lparam) noexcept;
		void OnMouseMove(LPARAM lparam) noexcept;

		LRESULT DefaultHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept;

		void ShowImpl();

		void HideImpl();

		void ProcessEventsImpl();

		void SetTitleImpl(std::string_view title);

		std::pair<std::uint32_t, std::uint32_t> GetWidthAndHeightImpl() const noexcept;

	public:
		using Base = core::BaseWindow<WindowsWindow>;
		using WindowCloseEvent = core::WindowCloseEvent<WindowsWindow>;
		using WindowResizeEvent = core::WindowResizeEvent<WindowsWindow>;
		using KeyboardEvent = core::KeyboardEvent<WindowsWindow>;
		using MouseButtonEvent = core::MouseButtonEvent<WindowsWindow>;
		using MouseMoveEvent = core::MouseMoveEvent<WindowsWindow>;

		WindowsWindow(std::string_view title, std::uint32_t width, std::uint32_t height);

		~WindowsWindow() noexcept;

		boost::uuids::uuid AttachMsgProcessor(MsgProcessor const& processor);

		boost::uuids::uuid StrictAttachMsgProcessor(MsgProcessor const& processor);

		void DetachBackMsgProcessor();

		void DetachMsgProcessor(boost::uuids::uuid const& uuid);

		HWND GetHWND() const noexcept;

	};

	export using WindowCloseEvent = WindowsWindow::WindowCloseEvent;
	export using WindowResizeEvent = WindowsWindow::WindowResizeEvent;
	export using KeyboardEvent = WindowsWindow::KeyboardEvent;
	export using MouseButtonEvent = WindowsWindow::MouseButtonEvent;
	export using MouseMoveEvent = WindowsWindow::MouseMoveEvent;

#endif 
}