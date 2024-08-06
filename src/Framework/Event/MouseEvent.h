#ifndef MOUSE_EVENT_H
#define MOUSE_EVENT_H

#include "CommonEvent.h"
#include "Framework/Core/MouseButton.h"

namespace Fyuu {

	class MouseMovedEvent final : public CommonEvent {

	public:

		MouseMovedEvent(float x, float y) 
			:m_mouse_x(x), m_mouse_y(y) {};

		float const& GetX() const noexcept { return m_mouse_x; }
		float const& GetY() const noexcept { return m_mouse_y; }

	private:

		float m_mouse_x, m_mouse_y;

	};

	class MouseScrolledEvent final : public CommonEvent {

	public:

		MouseScrolledEvent(float offset_x, float offset_y) 
			:m_offset_x(offset_x), m_offset_y(offset_y) {}

		float const& GetOffsetX() const noexcept { return m_offset_x; }
		float const& GetOffsetY() const noexcept { return m_offset_y; }

	private:

		float m_offset_x, m_offset_y;

	};

	enum MouseButtonEventType {

		MOUSE_BUTTON_EVENT_NULL = 0x0000,
		MOUSE_BUTTON_EVENT_PRESSED = 0x0001,
		MOUSE_BUTTON_EVENT_RELEASED = 0x0002,

	};

	template <MouseButtonEventType EventType>
	class MouseButtonEventTemplate final : CommonEvent {

	public:
		MouseButtonEventTemplate(MouseButton button) 
			:m_button(button), m_event_type(EventType) {}
		MouseButton const& GetButton() const noexcept { return m_button; }

	private:

		MouseButton m_button;
		MouseButtonEventType m_event_type;

	};

	using MouseButtonPressedEvent = MouseButtonEventTemplate<MOUSE_BUTTON_EVENT_PRESSED>;
	using MouseButtonReleasedEvent = MouseButtonEventTemplate<MOUSE_BUTTON_EVENT_RELEASED>;

}

#endif // !MOUSE_EVENT_H
