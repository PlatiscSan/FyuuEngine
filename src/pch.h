
#if _MSC_VER > 1928 
#include <__msvc_all_public_headers.hpp>
#endif

#if __GNUC__
#include <bits/stdc++.h>
#endif

#if _WIN32

	#include <Windows.h>
	#include <windowsx.h>

	#include <d3d12.h>
	#include <dxgi1_6.h>
	#include <d3d12sdklayers.h>
	#include <sdkddkver.h>
	#include <d3dcompiler.h>
	#include <wrl.h>

	#if _DEBUG
		#include <dxgidebug.h>
	#endif // _DEBUG

#elif __linux__


#endif // _WIN32

#include <vulkan/vulkan.h>
