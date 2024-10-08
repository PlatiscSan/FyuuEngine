#ifndef WINDOW_EVENT_H
#define WINDOW_EVENT_H

#include "Framework/Core/MessageBus.h"
#include "Framework/Core/Window.h"

namespace Fyuu::core {

	enum WindowEventType {

		WINDOW_EVENT_NULL = 0x0000,
		WINDOW_EVENT_CREATE = 0x0001,
		WINDOW_EVENT_DESTROY = 0x0002,
		WINDOW_EVENT_SIZE = 0x0005,
		WINDOW_EVENT_SET_FOCUS = 0x0007,
		WINDOW_EVENT_KILL_FOCUS = 0x0008,
		WINDOW_EVENT_CLOSE = 0x0010,
		WINDOW_EVENT_SHOWN = 0x0018,

	};

	template <WindowEventType EventType>
	class WindowEventTemplate final : public BaseEvent {
	public:

		WindowEventTemplate(Window* window)
			: m_event_type(EventType), m_window(window) {}

		WindowEventType const& GetEventType() const noexcept { return m_event_type; }

		Window* GetWindow() const noexcept { return m_window; }

	private:

		Window* m_window = nullptr;
		WindowEventType m_event_type = WINDOW_EVENT_NULL;

	};

	class WindowResizedEvent final : public BaseEvent {

	public:

		WindowResizedEvent(Window* window, std::uint32_t width, std::uint32_t height)
			:m_window(window), m_width(width), m_height(height) {}

		WindowEventType const& GetEventType() const noexcept { return m_event_type; }

		std::uint32_t const& GetWidth() const noexcept { return m_width; }
		std::uint32_t const& GetHeight() const noexcept { return m_height; }

	private:

		Window* m_window;
		std::uint32_t m_width, m_height;
		WindowEventType m_event_type = WINDOW_EVENT_SIZE;


	};

	using WindowCreateEvent = WindowEventTemplate<WINDOW_EVENT_CREATE>;
	using WindowDestroyEvent = WindowEventTemplate<WINDOW_EVENT_DESTROY>;
	using WindowSetFocusEvent = WindowEventTemplate<WINDOW_EVENT_SET_FOCUS>;
	using WindowKillFocusEvent = WindowEventTemplate<WINDOW_EVENT_KILL_FOCUS>;
	using WindowCloseEvent = WindowEventTemplate<WINDOW_EVENT_CLOSE>;
	using WindowShownEvent = WindowEventTemplate<WINDOW_EVENT_SHOWN>;

}

#endif // !WINDOW_EVENT_H
