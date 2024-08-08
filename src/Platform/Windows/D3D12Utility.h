#ifndef D3D12_UTILITY_H
#define D3D12_UTILITY_H

#include "WindowsUtility.h"

#include <functional>
#include <mutex>

#include <comdef.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>
#include <sdkddkver.h>
#include <d3dcompiler.h>
#include <wrl.h>

#if _DEBUG
	#include <DXGIDebug.h>
#endif // _DEBUG

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

namespace Fyuu::windows_util {

	using Microsoft::WRL::ComPtr;

	static constexpr std::size_t FRAME_BUFFER_COUNT = 3;

	inline void ThrowIfFailed(HRESULT expression) {

		if (FAILED(expression)) {

			_com_error error(expression);
			String msg = error.ErrorMessage();

		#if defined(_UNICODE) || defined(UNICODE)
			throw std::runtime_error(WStringToString(msg));
		#else
			throw std::runtime_error(msg);
		#endif // defined(_UNICODE) || defined(UNICODE)

		}

	}


}

#endif // !D3D12_UTILITY_H
