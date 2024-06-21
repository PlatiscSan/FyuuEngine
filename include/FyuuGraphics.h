#ifndef FYUU_GRAPHICS_H
#define FYUU_GRAPHICS_H

#include "APIControl.h"
#include "FyuuError.h"

#if defined(__cplusplus)
	#include <cstdint>
#else
	#include <stdint.h>
#endif // defined(__cplusplus)

typedef struct FyuuWindow {

}FyuuWindow;

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

	FYUU_API FyuuWindow* FyuuCreateWindow(char const* name);
	FYUU_API FyuuError_t FyuuSetWindowTitle(FyuuWindow* window, char const* title);
	FYUU_API FyuuError_t FyuuSetWindowSize(FyuuWindow* window, uint32_t width, uint32_t height);
	FYUU_API FyuuError_t FyuuSetWindowPosition(FyuuWindow* window, int x, int y);
	FYUU_API void FyuuDestroyWindow(FyuuWindow* window);


#if defined(__cplusplus)
}
#endif // defined(__cplusplus)

#endif // !FYUU_GRAPHICS_H
