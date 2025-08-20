module window;
namespace platform {
	IWindow& CreateMainWindow(std::string_view title, std::uint32_t width, std::uint32_t height) {
#ifdef WIN32
		static Win32Window window(title, width, height);
#else
		static GLFWWindow window(title, width, height);
#endif // WIN32
		return window;
	}
}