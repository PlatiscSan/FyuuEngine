#ifndef FYUU_EVENT_H
#define FYUU_EVENT_H

#include "FyuuGraphics.h"

typedef enum Fyuu_WindowEventType {
	FYUU_WINDOW_EVENT_NULL,
	FYUU_WINDOW_EVENT_CLOSE,
	FYUU_WINDOW_EVENT_DESTROY,
	FYUU_WINDOW_SET_FOCUS,
	FYUU_WINDOW_LOSE_FOCUS
}Fyuu_WindowEventType;

typedef struct Fyuu_WindowEvent {
	Fyuu_Window window;
	Fyuu_WindowEventType type;
}Fyuu_WindowEvent;

typedef void (__cdecl *Fyuu_WindowCallback)(Fyuu_WindowEvent e);

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

	FYUU_API void Fyuu_SubscribeWindowMessage(Fyuu_WindowCallback callback);

#if defined(__cplusplus)
}
#endif // defined(__cplusplus)

#endif // !FYUU_EVENT_H
