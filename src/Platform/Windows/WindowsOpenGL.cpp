
#include "pch.h"
#include "Renderer/OpenGL/OpenGLCore.h"
#include "WindowsAppInstance.h"
#include "WindowsUtility.h"

using namespace Fyuu::utility::windows;
using namespace Fyuu::core;

void Fyuu::graphics::opengl::LoadWGLExt() {

	HDC device_context = nullptr;
	PIXELFORMATDESCRIPTOR pixel_format = { 0 };
	HGLRC render_context = nullptr;

	WNDCLASSEX wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
	wcex.hInstance = WindowsAppInstance();
	wcex.lpfnWndProc = DefWindowProc;
	wcex.lpszClassName = TEXT("OpenGL init");

	RegisterClassEx(&wcex);

	HWND hwnd = CreateWindowEx(0, TEXT("OpenGL init"), TEXT(""), 0, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, WindowsAppInstance(), nullptr);
	if (!hwnd) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	device_context = GetDC(hwnd);
	if (!device_context) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	if (!SetPixelFormat(device_context, 1, &pixel_format)) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	render_context = wglCreateContext(device_context);
	if (!render_context) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	if (!wglMakeCurrent(device_context, render_context)) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	if (!wglChoosePixelFormatARB) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	if (!wglCreateContextAttribsARB) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	if (!wglSwapIntervalEXT) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	if (!glAttachShader) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
	if (!glBindBuffer)
	{
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
	if (!glBindVertexArray)
	{
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
	if (!glBufferData) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	if (!glCompileShader) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	if (!glCreateProgram) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	if (!glCreateShader) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
	if (!glDeleteBuffers) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
	if (!glDeleteProgram) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	if (!glDeleteShader) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)wglGetProcAddress("glDeleteVertexArrays");
	if (!glDeleteVertexArrays) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
	if (!glDetachShader) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
	if (!glEnableVertexAttribArray) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
	if (!glGenBuffers) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
	if (!glGenVertexArrays) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocation");
	if (!glGetAttribLocation) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
	if (!glGetProgramInfoLog) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
	if (!glGetProgramiv) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	if (!glGetShaderInfoLog) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	if (!glGetShaderiv) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	if (!glLinkProgram) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	if (!glShaderSource) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	if (!glUseProgram) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
	if (!glVertexAttribPointer) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)wglGetProcAddress("glBindAttribLocation");
	if (!glBindAttribLocation) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
	if (!glGetUniformLocation) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
	if (!glUniformMatrix4fv) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
	if (!glActiveTexture) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
	if (!glUniform1i) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
	if (!glGenerateMipmap) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glDisableVertexAttribArray");
	if (!glDisableVertexAttribArray) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glUniform3fv = (PFNGLUNIFORM3FVPROC)wglGetProcAddress("glUniform3fv");
	if (!glUniform3fv) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glUniform4fv = (PFNGLUNIFORM4FVPROC)wglGetProcAddress("glUniform4fv");
	if (!glUniform4fv) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	wglMakeCurrent(nullptr, nullptr);
	wglDeleteContext(render_context);
	render_context = nullptr;

	ReleaseDC(hwnd, device_context);
	device_context = nullptr;

	DestroyWindow(hwnd);
	UnregisterClass(TEXT("OpenGL init"), WindowsAppInstance());

}

void Fyuu::graphics::opengl::InitializeWindowsGLContext(HWND hwnd) {

	HDC device_context = GetDC(hwnd);
	if (!device_context) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	int pixel_format;
	UINT format_count;
	std::array<int, 19> attribute_list = {};

	attribute_list[0] = WGL_SUPPORT_OPENGL_ARB;
	attribute_list[1] = true;

	// Support for rendering to a window.
	attribute_list[2] = WGL_DRAW_TO_WINDOW_ARB;
	attribute_list[3] = true;

	// Support for hardware acceleration.
	attribute_list[4] = WGL_ACCELERATION_ARB;
	attribute_list[5] = WGL_FULL_ACCELERATION_ARB;

	// Support for 24bit color.
	attribute_list[6] = WGL_COLOR_BITS_ARB;
	attribute_list[7] = 24;

	// Support for 24 bit depth buffer.
	attribute_list[8] = WGL_DEPTH_BITS_ARB;
	attribute_list[9] = 24;

	// Support for double buffer.
	attribute_list[10] = WGL_DOUBLE_BUFFER_ARB;
	attribute_list[11] = true;

	// Support for swapping front and back buffer.
	attribute_list[12] = WGL_SWAP_METHOD_ARB;
	attribute_list[13] = WGL_SWAP_EXCHANGE_ARB;

	// Support for the RGBA pixel type.
	attribute_list[14] = WGL_PIXEL_TYPE_ARB;
	attribute_list[15] = WGL_TYPE_RGBA_ARB;

	// Support for a 8 bit stencil buffer.
	attribute_list[16] = WGL_STENCIL_BITS_ARB;
	attribute_list[17] = 8;

	// Null terminate the attribute list.
	attribute_list[18] = 0;


	// Query for a pixel format that fits the attributes we want.
	if (!wglChoosePixelFormatARB(device_context, attribute_list.data(), nullptr, 1, &pixel_format, &format_count)) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	PIXELFORMATDESCRIPTOR pixel_format_descriptor = { .nSize = sizeof(PIXELFORMATDESCRIPTOR) };
	DescribePixelFormat(device_context, pixel_format, sizeof(PIXELFORMATDESCRIPTOR), &pixel_format_descriptor);

	// If the video card/display can handle our desired pixel format then we set it as the current one.
	if (!SetPixelFormat(device_context, pixel_format, &pixel_format_descriptor)) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	std::memset(attribute_list.data(), 0, attribute_list.size());

	// Set the 4.0 version of OpenGL in the attribute list.
	attribute_list[0] = WGL_CONTEXT_MAJOR_VERSION_ARB;
	attribute_list[1] = 4;
	attribute_list[2] = WGL_CONTEXT_MINOR_VERSION_ARB;
	attribute_list[3] = 5;

	// Null terminate the attribute list.
	attribute_list[4] = 0;

	// Create a OpenGL 4.5 rendering context.
	HGLRC rendering_context = wglCreateContextAttribsARB(device_context, 0, attribute_list.data());
	if (!rendering_context) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	// Set the rendering context to active.
	if (!wglMakeCurrent(device_context, rendering_context)) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	glClearDepth(1.0f);

	// Enable depth testing.
	glEnable(GL_DEPTH_TEST);

	// Set the polygon winding to front facing for the left handed system.
	glFrontFace(GL_CW);

	// Enable back face culling.
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	SetGLDeviceContext(device_context);
	SetGLRenderingContext(rendering_context);

}