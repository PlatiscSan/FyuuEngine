#ifndef API_CONTROL_H
#define API_CONTROL_H

#if defined(_WIN32)

	#ifdef DLLEXPORT
		#define FYUU_API __declspec(dllexport)
	#else	
		#define FYUU_API __declspec(dllimport)
	#endif // DLLEXPORT

	#define FYUU_CALL __cdecl

#elif defined(__GNUC__)

	#ifdef SOEXPORT
		#define FYUU_API __attribute__((visibility("default")))
	#else
		#define FYUU_API
	#endif // SOEXPORT

	#define FYUU_CALL  __attribute__((__cdecl__))

#endif // defined(_WIN32)

#endif // !API_CONTROL_H
