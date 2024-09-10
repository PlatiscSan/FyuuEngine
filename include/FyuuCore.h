#ifndef FYUU_CORE_H
#define FYUU_CORE_H

#include "APIControl.h"

#ifdef __cplusplus
	#include <cstdint>
#else
	#include <stdint.h>
#endif // __cplusplus


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	FYUU_API int FYUU_CALL Fyuu_RunApplication(int argc, char** argv);

	FYUU_API void* FYUU_CALL Fyuu_Malloc(size_t size_in_bytes);
	FYUU_API void FYUU_CALL Fyuu_Free(void* p);

	FYUU_API char const* FYUU_CALL Fyuu_GetLastError();


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !FYUU_CORE_H
