
#include "OpenGLUtility.h"

bool LoadGL(HWND hwnd) {

	HDC device_context = { 0 };
	PIXELFORMATDESCRIPTOR pixel_format = { 0 };
	HGLRC render_context = { 0 };

	device_context = GetDC(hwnd);
	if (!device_context) {
		return false;
	}

	if (!SetPixelFormat(device_context, 1, &pixel_format)) {
		return false;
	}

	render_context = wglCreateContext(device_context);
	if (!render_context) {
		return false;
	}

	if (!wglMakeCurrent(device_context, render_context)) {
		return false;
	}







	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(render_context);
	render_context = NULL;

	ReleaseDC(hwnd, device_context);
	device_context = NULL;

	return true;

}
