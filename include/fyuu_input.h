#pragma once
#include "fyuu_graphics.h"

#if !defined(__cplusplus)
#include <stdbool.h>
#endif // !defined(__cplusplus)

DECLARE_FYUU_FLAG_AUTO(InputCommand, Forward, Backward, Left, Right, Jump, Squat, Sprint, Attack, FreeCamera)
DECLARE_FYUU_ENUM(
	Key,
	Back,
	Tab,
	Enter,
	Escape,
	Space,
	A, B, C, D, E, F, G, H, I, J, K, L, M,
	N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

	D0, D1, D2, D3, D4, D5, D6, D7, D8, D9,

	F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

	Left, Right, Up, Down,

	LeftShift, RightShift,
	LeftCtrl, RightCtrl,
	LeftAlt, RightAlt,
	LeftWin, RightWin,

	CapsLock, NumLock, ScrollLock,
	PrintScreen, Pause,
	Insert, Delete, Home, End, PageUp, PageDown
)

DECLARE_FYUU_CLASS_C(FyuuInputSystem)

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCreateInputSystem(
	FyuuInputSystem* input_system
) NO_EXCEPT;
DECLARE_FYUU_DESTROY(InputSystem)

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCaptureSurfaceEvent(
	FyuuInputSystem* input_system,
	FyuuSurface* surface
) NO_EXCEPT;

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuReleaseEventHandle(
	FyuuInputSystem* input_system,
	void* handle
) NO_EXCEPT;

EXTERN_C DLL_API char const* DLL_CALL FyuuGetLastInputSystemErrorMessage() NO_EXCEPT;

#if defined(__cplusplus)
namespace fyuu_engine {
	
	DECLARE_FYUU_CLASS(InputSystem) {

		InputSystem() {
			auto result
				= static_cast<ErrorCode>(FyuuCreateInputSystem(this));
			if (result != ErrorCode::Success) {
				throw std::runtime_error(FyuuGetLastInputSystemErrorMessage());
			}
		}

		void CaptureSurfaceEvent(
			Surface& surface
		) {
			auto result
				= static_cast<ErrorCode>(
					FyuuCaptureSurfaceEvent(
						this,
						&surface
					));
			if (result != ErrorCode::Success) {
				throw std::runtime_error(FyuuGetLastInputSystemErrorMessage());
			}
		}

		void ReleaseEventHandle(
			Surface& surface
		) {
			auto result
				= static_cast<ErrorCode>(
					FyuuReleaseEventHandle(
						this,
						&surface
					));
			if (result != ErrorCode::Success) {
				throw std::runtime_error(FyuuGetLastInputSystemErrorMessage());
			}
		}

		FYUU_CLASS_COMMON_PARTS(InputSystem)
	};

}
#endif // defined(__cplusplus)

