#ifndef WINDOWS_APPLICATION_H
#define WINDOWS_APPLICATION_H

#include "../Interface/IApplication.h"
#include "../Common/Singleton.h"

#include <Windows.h>
#include <windowsx.h>

namespace Fyuu {

	class WindowsApplication final : public IApplication, public Singleton<WindowsApplication> {

		friend class Singleton<WindowsApplication>;

	public:

		void Tick();

		HINSTANCE const& GetApplicationHandle() const noexcept;

	private:

		WindowsApplication(int argc, char** argv);

	private:

		HINSTANCE m_handle = GetModuleHandle(nullptr);

	};

}

#endif // !WINDOWS_APPLICATION_H
