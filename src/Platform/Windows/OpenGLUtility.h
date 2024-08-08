
#ifndef OPENGL_UTILITY_H
#define OPENGL_UTILITY_H

#include <Windows.h>
#include <gl/glcorearb.h>

#ifdef __cplusplus
	#include <cmath>
	#include <cstdbool>

#else
	#include <math.h>
	#include <stdbool.h>
#endif // __cplusplus

#pragma comment(lib, "opengl32.lib")

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	extern bool LoadGL(HWND hwnd);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !OPENGL_UTILITY_H
