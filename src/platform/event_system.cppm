export module event_system;
export import window_interface;
import std;

export namespace platform {
    struct BaseEvent {
        IWindow* source;
        BaseEvent(IWindow* window)
            : source(window) {

        }
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
            : BaseEvent(window), width(w), height(h) {
        }
    };

    struct KeyEvent : public BaseEvent {
        enum class Action : std::uint8_t { Press, Release, Repeat };
        int key_code;
        Action action;

        KeyEvent(IWindow* window, int key, Action action)
            : BaseEvent(window), key_code(key), action(action) {
        }

    };

    struct MouseMoveEvent : public BaseEvent {
        double pos_x;
        double pos_y;

        MouseMoveEvent(IWindow* window, double x, double y)
            : BaseEvent(window), pos_x(x), pos_y(y) {
        }
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
}