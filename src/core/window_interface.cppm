export module window_interface;
export import message_bus;
import std;

export namespace core {
	class IWindow {
	public:
        struct BaseEvent {
            IWindow* source;
            BaseEvent(IWindow* window)
                : source(window) {

            }
            virtual ~BaseEvent() noexcept = default;
        };

        struct WindowCloseEvent : public BaseEvent {
            WindowCloseEvent(IWindow* window)
                : BaseEvent(window) {

            }
        };

        struct WindowResizeEvent : public BaseEvent {
            std::uint32_t width;
            std::uint32_t height;

            WindowResizeEvent(IWindow* window, std::uint32_t w, std::uint32_t h) 
                : BaseEvent(window), width(w), height(h) {}
        };

        struct KeyEvent : public BaseEvent {
            enum class Action : std::uint8_t { Press, Release, Repeat };
            int key_code;
            Action action;

            KeyEvent(IWindow* window, int key, Action action) 
                : BaseEvent(window), key_code(key), action(action) {}

        };

        struct MouseMoveEvent : public BaseEvent {
            double pos_x;
            double pos_y;

            MouseMoveEvent(IWindow* window, double x, double y) 
                : BaseEvent(window), pos_x(x), pos_y(y) {}
        };

        struct MouseButtonEvent : public BaseEvent {
            enum class Action : std::uint8_t { Press, Release };
            enum class Button : std::uint8_t { Left, Right, Middle };

            double pos_x;
            double pos_y;
            Button button;
            Action action;


            MouseButtonEvent(IWindow* window, double x, double y, Button btn, Action action)
                : BaseEvent(window), pos_x(x), pos_y(y), button(btn), action(action) {
            }

        };

		virtual ~IWindow() noexcept = default;
		virtual void Create(std::string_view title, std::uint32_t width, std::uint32_t height) = 0;
		virtual void Show() = 0;
		virtual void Hide() = 0;
		virtual void Close() noexcept = 0;

		virtual void ProcessEvents() = 0;
		virtual void SetTitle(std::string_view title) = 0;
		virtual std::pair<std::uint32_t, std::uint32_t> GetWidthAndHeight() const noexcept = 0;
		virtual void* Native() const noexcept = 0;

		virtual util::MessageBus* GetMessageBus() noexcept = 0;

	};

}