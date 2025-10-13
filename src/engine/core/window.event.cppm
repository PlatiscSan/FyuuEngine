export module window:event;
export import pointer_wrapper;
import :interface;
import std;

namespace fyuu_engine::core {

	export template <class Window> class BaseEvent {
	private:
		util::PointerWrapper<Window> m_source;

	public:
		BaseEvent(util::PointerWrapper<Window> const& source)
			: m_source(util::MakeReferred(source, false)) {

		}

		util::PointerWrapper<Window> GetSource() const noexcept {
			return m_source;
		}

	};

	export template <class Window> class WindowCloseEvent
		: public BaseEvent<Window> {
	public:
		using BaseEvent = BaseEvent<Window>;

		WindowCloseEvent(util::PointerWrapper<Window> const& source)
			: BaseEvent(source) {
		}

	};

	export template <class Window> class WindowResizeEvent
		: public BaseEvent<Window> {
	private:
		std::uint32_t m_width;
		std::uint32_t m_height;

	public:
		using BaseEvent = BaseEvent<Window>;

		WindowResizeEvent(util::PointerWrapper<Window> const& source, std::uint32_t w, std::uint32_t h)
			: BaseEvent(source), m_width(w), m_height(h) {
		}

		std::uint32_t GetWidth() const noexcept {
			return m_width;
		}

		std::uint32_t GetHeight() const noexcept {
			return m_height;
		}


	};

	export template <class Window> class KeyboardEvent 
		: public BaseEvent<Window> {
	public:
		enum class Action : std::uint8_t { Press, Release, Repeat };
		using BaseEvent = BaseEvent<Window>;

	private:
		int m_key_code;
		Action m_action;

	public:
		KeyboardEvent(util::PointerWrapper<Window> const& source, int key, Action action)
			: BaseEvent(source), m_key_code(key), m_action(action) {
		}

		int GetKeyCode() const noexcept {
			return m_key_code;
		}

		Action GetAction() const noexcept {
			return m_action;
		}

	};

	export template <class Window> class MouseMoveEvent 
		: public BaseEvent<Window> {
	private:
		double m_pos_x;
		double m_pos_y;

	public:
		using BaseEvent = BaseEvent<Window>;

		MouseMoveEvent(util::PointerWrapper<Window> const& source, double x, double y)
			: BaseEvent(source), m_pos_x(x), m_pos_y(y) {
		}

		double GetX() const noexcept {
			return m_pos_x;
		}

		double GetY() const noexcept {
			return m_pos_y;
		}

	};

	export template <class Window> class MouseButtonEvent 
		: public BaseEvent<Window> {
	public:
		enum class Action : std::uint8_t { Press, Release };
		enum class Button : std::uint8_t { Left, Right, Middle };
		using BaseEvent = BaseEvent<Window>;

	private:
		double m_pos_x;
		double m_pos_y;
		Button m_button;
		Action m_action;

	public:
		MouseButtonEvent(util::PointerWrapper<Window> const& source, double x, double y, Button btn, Action action)
			: BaseEvent(source), m_pos_x(x), m_pos_y(y), m_button(btn), m_action(action) {
		}

		double GetX() const noexcept {
			return m_pos_x;
		}

		double GetY() const noexcept {
			return m_pos_y;
		}

		Button GetButton() const noexcept {
			return m_button;
		}

		Action GetAction() const noexcept {
			return m_action;
		}

	};


}