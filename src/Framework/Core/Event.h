#ifndef EVENT_H
#define EVENT_H

#include "MessageBus.h"
#include "Window.h"
#include "MouseButton.h"
#include "Keycode.h"

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
	struct WindowEventTemplate final : public BaseEvent {

		Window* window;
		WindowEventType const event_type = EventType;

	};

	struct WindowResizedEvent final : public BaseEvent {

		Window* window;
		std::uint32_t width, height;
		WindowEventType const m_event_type = WINDOW_EVENT_SIZE;

	};

	using WindowCreateEvent = WindowEventTemplate<WINDOW_EVENT_CREATE>;
	using WindowDestroyEvent = WindowEventTemplate<WINDOW_EVENT_DESTROY>;
	using WindowSetFocusEvent = WindowEventTemplate<WINDOW_EVENT_SET_FOCUS>;
	using WindowKillFocusEvent = WindowEventTemplate<WINDOW_EVENT_KILL_FOCUS>;
	using WindowCloseEvent = WindowEventTemplate<WINDOW_EVENT_CLOSE>;
	using WindowShownEvent = WindowEventTemplate<WINDOW_EVENT_SHOWN>;


	struct MouseMovedEvent final : public BaseEvent {

		float mouse_x, mouse_y;

	};

	struct MouseScrolledEvent final : public BaseEvent {

		float offset_x, offset_y;

	};

	enum MouseButtonEventType {

		MOUSE_BUTTON_EVENT_NULL = 0x0000,
		MOUSE_BUTTON_EVENT_PRESSED = 0x0001,
		MOUSE_BUTTON_EVENT_RELEASED = 0x0002,

	};

	template <MouseButtonEventType EventType>
	struct MouseButtonEventTemplate final : BaseEvent {

		core::MouseButton m_button;
		MouseButtonEventType const m_event_type = EventType;

	};

	using MouseButtonPressedEvent = MouseButtonEventTemplate<MOUSE_BUTTON_EVENT_PRESSED>;
	using MouseButtonReleasedEvent = MouseButtonEventTemplate<MOUSE_BUTTON_EVENT_RELEASED>;

}

#endif // !EVENT_H
