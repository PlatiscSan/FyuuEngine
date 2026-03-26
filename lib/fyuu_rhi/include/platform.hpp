#pragma once
#if defined(_WIN32)
#include <Windows.h>
#include <directx/d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <comdef.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <wayland-client-core.h>
#include <wayland-util.h>
#elif defined(__ANDROID__)
#include <android/native_window.h>
#endif // defined(_WIN32)