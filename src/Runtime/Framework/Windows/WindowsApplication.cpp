
#include "pch.h"
#include "WindowsApplication.h"
#include "WindowsWindowManager.h"

using namespace Fyuu;

Fyuu::WindowsApplication::WindowsApplication(int argc, char** argv)
	:IApplication(argc, argv) {
	
}

void Fyuu::WindowsApplication::Tick() {

	MSG msg = {};
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

}

HINSTANCE const& Fyuu::WindowsApplication::GetApplicationHandle() const noexcept {
	return m_handle;
}
