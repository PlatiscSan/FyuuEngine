#ifndef FYUU_APP_H
#define FYUU_APP_H

#include "APIControl.h"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

	
	FYUU_API void FyuuInit(int argc, char* argv[]);

	FYUU_API void FyuuTick();

	FYUU_API bool IsQuit();

	FYUU_API void FyuuQuit();



#if defined(__cplusplus)
}
#endif // defined(__cplusplus)


#endif // !FYUU_APP_H
