
#include "pch.h"
#include "OpenGLDevice.h"

using namespace Fyuu;
using namespace Fyuu::windows_util;


Fyuu::OpenGLDevice::OpenGLDevice(WindowsWindow* window) {

	if (!LoadGL(window->GetWindowHandle())) {
		throw std::runtime_error("Failed to initialize OpenGL");
	}

	m_device_context = GetDC(window->GetWindowHandle());
	if (!m_device_context) {
		throw std::runtime_error(GetLastErrorFromWinAPI());
	}
	int pixel_format = 0;
	PIXELFORMATDESCRIPTOR pixel_format_descriptor{};

}
