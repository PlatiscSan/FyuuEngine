#ifndef KEYBOARD_EVENT_H
#define KEYBOARD_EVENT_H

#include "Framework/Core/MessageBus.h"
#include "Framework/Core/Keycode.h"

namespace Fyuu::core {

	enum KeyboardEventType {

		KEYBOARD_EVENT_NULL = 0x0000,
		KEYBOARD_EVENT_PRESSED = 0x0001,
		KEYBOARD_EVENT_RELEASED = 0x0002,

	};

	template <KeyboardEventType EventType>
	class KeyboardEventTemplate final : public BaseEvent {
	public:

		KeyboardEventTemplate(core::Keycode keycode)
			: m_event_type(EventType), m_keycode(keycode) {}

		KeyboardEventType const& GetEventType() const noexcept {
			return m_event_type;
		}

		core::Keycode const& GetKeycode() const noexcept {
			return m_keycode;
		}

	private:

		KeyboardEventType m_event_type;
		core::Keycode m_keycode;

	};

	using KeyPressedEvent = KeyboardEventTemplate<KEYBOARD_EVENT_PRESSED>;
	using KeyReleasedEvent = KeyboardEventTemplate<KEYBOARD_EVENT_RELEASED>;

}

#endif // !KEYBOARD_EVENT_H+H
