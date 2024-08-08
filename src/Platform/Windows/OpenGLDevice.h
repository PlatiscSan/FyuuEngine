#ifndef OPENGL_DEVICE_H
#define OPENGL_DEVICE_H

#include "Framework/Renderer/GraphicsDevice.h"
#include "WindowsWindow.h"
#include "WindowsUtility.h"
#include "OpenGLUtility.h"

namespace Fyuu {

	class OpenGLDevice final : public GraphicsDevice {

	public:

		OpenGLDevice(WindowsWindow* window);

	

	private:

		HDC m_device_context = nullptr;
		HGLRC m_rendering_context = nullptr;

	};

}

#endif // !OPENGL_DEVICE_H
