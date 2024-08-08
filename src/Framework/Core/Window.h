#ifndef WINDOW_H
#define WINDOW_H

#include <functional>
#include <string>

namespace Fyuu {

	class Window {

	public:
		
		Window(std::string const& name)
			:m_name(name), m_width(0), m_height(0) {}
		Window(Window const&) = delete;
		Window operator=(Window const&) = delete;

		virtual ~Window() noexcept = default;

		std::uint32_t const& GetWidth() const noexcept { return m_width; }
		std::uint32_t const& GetHeight() const noexcept { return m_height; }

		virtual void SetTitle(std::string const& title) = 0;
		virtual void SetSize(std::uint32_t width, std::uint32_t height) = 0;
		virtual void SetPosition(int x, int y) = 0;
		virtual void Show() = 0;

		virtual void* GetNativeWindow() const noexcept = 0;

		using WindowPtr = std::shared_ptr<Window>;

	protected:

		std::uint32_t m_width, m_height;
		std::string m_name;

	};

}

#endif // !WINDOW_H
