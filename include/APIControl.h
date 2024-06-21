#ifndef API_CONTROL_H
#define API_CONTROL_H

#if defined(_WIN32)

	#if defined(DLL_EXPORT)
		#define FYUU_API __declspec(dllexport)
	#else
		#define FYUU_API __declspec(dllimport)
	#endif // defined(DLL_EXPORT)

#elif defined(__GNUC__) && defined(__linux__)  

	#if defined(SO_EXPORT)  
		#define FYUU_API __attribute__((visibility("default")))  
	#else    
		#define FYUU_API  
	#endif // defined(SO_EXPORT)  

#else

	#define FYUU_API 

#endif // defined(_WIN32) || defined(__GNUC__) && defined(__linux__)



#endif // !API_CONTROL_H
