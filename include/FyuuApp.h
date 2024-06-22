#ifndef FYUU_APP_H
#define FYUU_APP_H

#include "APIControl.h"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

	
	FYUU_API void Fyuu_Init(int argc, char* argv[]);

	FYUU_API void Fyuu_Tick();

	FYUU_API bool Fyuu_IsQuit();

	FYUU_API void Fyuu_Quit();



#if defined(__cplusplus)
}
#endif // defined(__cplusplus)


#endif // !FYUU_APP_H
