#pragma once

#if defined(_MSC_VER)
	#define DLL_COMPILER_MSVC
#elif defined(__GNUC__)
	#define DLL_COMPILER_GCC
#elif defined(__clang__)
	#define DLL_COMPILER_CLANG
#endif

#if defined(_WIN32) || defined(_WIN64)
	#define DLL_PLATFORM_WINDOWS
#elif defined(__linux__)
	#define DLL_PLATFORM_LINUX
#elif defined(__APPLE__) && defined(__MACH__)
	#define DLL_PLATFORM_OSX
#endif

#if defined(DLL_PLATFORM_WINDOWS)
	#ifdef DLL_EXPORTS
		#define DLL_API __declspec(dllexport)
	#else
		#define DLL_API __declspec(dllimport)
	#endif
	#define DLL_CALL __stdcall
#else
	#if defined(DLL_COMPILER_GCC) || defined(DLL_COMPILER_CLANG)
		#ifdef DLL_EXPORTS
			#define DLL_API __attribute__((visibility("default")))
		#else
			#define DLL_API
		#endif
	#else
		#define DLL_API
	#endif
	#define DLL_CALL
#endif

#ifdef __cplusplus
	#define DLL_CLASS DLL_API
#else
	#define DLL_CLASS
#endif

#ifdef __cplusplus
	#define EXTERN_C extern "C"
#else
	#define EXTERN_C
#endif

#ifdef STATIC_LIB
	#undef DLL_API
	#define DLL_API
	#undef DLL_CALL
	#define DLL_CALL
#endif

#ifdef __cplusplus
	#define NO_EXCEPT noexcept
#else
	#define NO_EXCEPT
#endif // __cplusplus
