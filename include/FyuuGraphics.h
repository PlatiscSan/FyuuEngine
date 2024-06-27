#ifndef FYUU_GRAPHICS_H
#define FYUU_GRAPHICS_H

#include "APIControl.h"
#include "FyuuError.h"

#if defined(__cplusplus)
	#include <cstdint>
#else
	#include <stdint.h>
#endif // defined(__cplusplus)

typedef void* Fyuu_Window;

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

	FYUU_API Fyuu_Window Fyuu_CreateWindow(char const* name);
	FYUU_API Fyuu_Window Fyuu_FindWindow(char const* name);
	FYUU_API Fyuu_error_t Fyuu_SetWindowTitle(Fyuu_Window window, char const* title);
	FYUU_API Fyuu_error_t Fyuu_SetWindowSize(Fyuu_Window window, uint32_t width, uint32_t height);
	FYUU_API Fyuu_error_t Fyuu_SetWindowPosition(Fyuu_Window window, int x, int y);
	FYUU_API Fyuu_error_t Fyuu_ShowWindow(Fyuu_Window window);
	FYUU_API void Fyuu_DestroyWindow(Fyuu_Window window);


#if defined(__cplusplus)
}
#endif // defined(__cplusplus)

#endif // !FYUU_GRAPHICS_H
