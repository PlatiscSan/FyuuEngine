#ifndef BASE_WINDOW_H
#define BASE_WINDOW_H

#include <string>
#include <cstdint>

namespace Fyuu {
	
	class BaseWindow {

	public:

		BaseWindow(std::string name) {
			m_name = std::move(name);
		}

		virtual void SetTitle(std::string title) = 0;
		virtual void SetSize(std::uint32_t width, std::uint32_t height) = 0;
		virtual void SetPosition(int x, int y) = 0;
		virtual void Show() = 0;

		std::string const& GetName() const noexcept {
			return m_name;
		}

	protected:

		std::string m_name{};
		std::uint32_t m_width = 0;
		std::uint32_t m_height = 0;
		int m_x = 0;
		int m_y = 0;

	};

}

#endif // !BASE_WINDOW_H
