#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/system/system_error.hpp>

#include "fyuu_graphics.h"
#include "helper_macros.h"

import std;
import fyuu_rhi;
import fyuu_engine.rhi_error;
import plastic.synchronized_function;

#if defined(_WIN32)
using WindowClassName = std::array<TCHAR, 37u>;
#endif // defined(_WIN32)

struct FyuuSurface::Implementation {
	fyuu_rhi::Surface rhi_obj;
#if defined(_WIN32)
	WindowClassName wc_name{};
	HWND window_handle = nullptr;
#elif defined(__linux__)
	struct Wayland {
		wl_display* display = nullptr;
		wl_surface* surface = nullptr;
	};
	struct Xlib {
		display* display = nullptr;
		Window window;
	}
	std::variant<std::monostate, Xlib, Wayland> handle;
#elif defined(__ANDROID__)
	ANativeWindow* window_handle = nullptr;
#endif // defined(_WIN32)
};


#if defined(_WIN32)

static WindowClassName GenerateClassName() {

	static plastic::concurrency::SynchronizedFunction<boost::uuids::uuid()> GenerateUUID = []() {
		static boost::uuids::random_generator gen;
		return gen();
		};

	WindowClassName classname{};
	boost::uuids::to_chars(GenerateUUID(), classname.begin());
	return classname;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept {
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

static FyuuErrorCode FyuuCreateSurfaceImpl(
	FyuuSurface* surface,
	FyuuPhysicalDevice* physical_device,
	uint32_t width,
	uint32_t height,
	FyuuSurfaceFlag flags
) {

	surface->impl->wc_name = GenerateClassName();
	HINSTANCE instance = GetModuleHandle(nullptr);

	WNDCLASSEX wc{};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = instance;
	wc.lpszClassName = surface->impl->wc_name.data();

	if (!RegisterClassEx(&wc)) {
		throw boost::system::system_error(
			GetLastError(),
			boost::system::system_category()
		);
	}

	surface->impl->window_handle = CreateWindowEx(
		0,
		wc.lpszClassName,
		TEXT("Default title"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		nullptr,
		nullptr,
		instance,
		surface
	);

	if (!surface->impl->window_handle) {
		UnregisterClass(surface->impl->wc_name.data(), instance);
		throw boost::system::system_error(
			GetLastError(),
			boost::system::system_category()
		);
	}

	ShowWindow(surface->impl->window_handle, SW_SHOW);
	UpdateWindow(surface->impl->window_handle);

	MSG msg;

	while(PeekMessage(
		&msg,
		surface->impl->window_handle,
		0,
		0,
		PM_REMOVE
	)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	auto physical_device_impl = reinterpret_cast<fyuu_rhi::PhysicalDevice*>(physical_device->impl);

	try {
		return std::visit(
			[window_handle = surface->impl->window_handle, &surface = surface->impl->rhi_obj](auto&& physical_device) {
				using PhysicalDevice = std::decay_t<decltype(physical_device)>;
				if constexpr (!std::same_as<std::monostate, PhysicalDevice>) {

					static constexpr std::size_t index
						= fyuu_rhi::RHITypeIndex<std::remove_pointer_t<decltype(physical_device_impl)>, PhysicalDevice>;

					surface.emplace<index>(
						physical_device,
						window_handle
					);

					return FyuuErrorCode::FyuuErrorCode_Success;

				}
				else {
					return FyuuErrorCode::FyuuErrorCode_Unsupported;
				}
			},
			*physical_device_impl
		);
	}
	catch (std::exception const&) {
#if defined(_WIN32)
		if (surface->impl->window_handle) {
			DestroyWindow(surface->impl->window_handle);
			UnregisterClass(surface->impl->wc_name.data(), instance);
			surface->impl->window_handle = nullptr;
		}
#elif defined(__linux__)
		std::visit(
			[](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<std::monostate, FyuuSurface::Implementation::Wayland>) {
					if (handle) {
						wl_surface_destroy(handle.surface);
						handle.surface = nullptr;
					}
				}
				else if constexpr (std::same_as<std::monostate, FyuuSurface::Implementation::Xlib>) {
					if (handle) {
						XDestroyWindow(handle.display, handle.window);
						handle.window = 0;
					}
				}
				else {
					// Do nothing for std::monostate
				}
			},
			surface->impl->handle
		);
#elif defined(__ANDROID__)
		if (surface->impl->window_handle) {
			// Note: ANativeWindow is usually managed elsewhere; do not destroy it here.
			surface->impl->window_handle = nullptr;
		}
#endif // defined(_WIN32)
		throw;
	}

}
#elif defined(__linux__)
static FyuuErrorCode FyuuCreateSurfaceImpl(
	FyuuSurface* surface,
	FyuuPhysicalDevice* physical_device,
	uint32_t width,
	uint32_t height,
	FyuuSurfaceFlag flags
) {

}
#elif defined(__ANDROID__)
static FyuuErrorCode FyuuCreateSurfaceImpl(
	FyuuSurface* surface,
	FyuuPhysicalDevice* physical_device,
	uint32_t width,
	uint32_t height,
	FyuuSurfaceFlag flags
) {

}
#endif // defined(_WIN32)


EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCreateSurface(
	FyuuSurface* surface,
	FyuuPhysicalDevice* physical_device,
	uint32_t width,
	uint32_t height,
	FyuuSurfaceFlag flags
) NO_EXCEPT try {

	if (!surface || !physical_device) {
		return FyuuErrorCode_InvalidPointer;
	}

	if (!surface->impl) {
		surface->impl = new FyuuSurface::Implementation();
	}

	return FyuuCreateSurfaceImpl(
		surface,
		physical_device,
		width,
		height,
		flags
	);

}
CATCH_COMMON_EXCEPTION(surface)

EXTERN_C DLL_API void DLL_CALL FyuuDestroySurface(FyuuSurface* obj) NO_EXCEPT {
	if (obj) {
#if defined(_WIN32)
		if (obj->impl->window_handle) {
			HINSTANCE instance = GetModuleHandle(nullptr);
			DestroyWindow(obj->impl->window_handle);
			UnregisterClass(obj->impl->wc_name.data(), instance);
			obj->impl->window_handle = nullptr;
		}
#elif defined(__linux__)
		std::visit(
			[](auto&& handle) {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<std::monostate, FyuuSurface::Implementation::Wayland>) {
					if (handle) {
						wl_surface_destroy(handle.surface);
						handle.surface = nullptr;
					}
				}
				else if constexpr (std::same_as<std::monostate, FyuuSurface::Implementation::Xlib>) {
					if (handle) {
						XDestroyWindow(handle.display, handle.window);
						handle.window = 0;
					}
				}
				else {
					// Do nothing for std::monostate
				}
			},
			obj->impl->handle
		);
#elif defined(__ANDROID__)
		if (obj->impl->window_handle) {
			// Note: ANativeWindow is usually managed elsewhere; do not destroy it here.
			obj->impl->window_handle = nullptr;
		}
#endif // defined(_WIN32)

		delete obj->impl;
	}
}

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuSetSurfaceTitle(
	FyuuSurface* surface,
#if defined(WIN32)
	TCHAR const* title
#else
	char const* title
#endif // defined(WIN32)
) NO_EXCEPT {

	if (SetWindowText(surface->impl->window_handle, title)) {
		return FyuuErrorCode::FyuuErrorCode_Success;
	}

	boost::system::error_code error(
		GetLastError(),
		boost::system::system_category()
	);
	fyuu_engine::rhi::SetLastRHIErrorMessage(error.message());

	return FyuuErrorCode::FyuuErrorCode_SystemError;

}