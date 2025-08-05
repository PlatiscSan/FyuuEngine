export module window;
export import message_bus;
import std;

export import :interface;
export import :win32;
export import :glwf;

namespace platform {
	export IWindow& CreateMainWindow(std::string_view title, std::uint32_t width, std::uint32_t height);
}