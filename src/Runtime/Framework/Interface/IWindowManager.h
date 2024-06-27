#ifndef IWINDOW_MANAGER_H
#define IWINDOW_MANAGER_H

#include <concepts>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace Fyuu {

	class IWindow {

	public:

		IWindow(std::string name) :m_name(name) {}
		virtual ~IWindow() noexcept = default;

		virtual void SetTitle(std::string title) = 0;
		virtual void SetSize(std::uint32_t width, std::uint32_t height) = 0;
		virtual void SetPosition(int x, int y) = 0;
		virtual void Show() = 0;

		std::string const& GetName() const noexcept {
			return m_name;
		}

	protected:

		std::string m_name{};
		std::uint32_t m_width = 0, m_height = 0;
		int m_x = 0, m_y = 0;
		


	};

	class IWindowManager {

	public:

		IWindowManager() = default;
		virtual ~IWindowManager() noexcept = default;

		virtual IWindow* Create(std::string name) = 0;
		virtual IWindow* Find(std::string name) = 0;
		virtual void Destroy(IWindow* window) = 0;
		virtual void Destroy(std::string name) = 0;

	};

}

#endif // !IWINDOW_MANAGER_H
