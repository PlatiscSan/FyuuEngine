#ifndef D3D12_CORE_H
#define D3D12_CORE_H

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

#include "Platform/Windows/WindowsUtility.h"

namespace Fyuu::graphics::d3d12 {

	inline void ThrowIfFailed(HRESULT expression) {

		if (FAILED(expression)) {

			_com_error error(expression);
			utility::windows::String msg = error.ErrorMessage();

		#if defined(_UNICODE) || defined(UNICODE)
			throw std::runtime_error(utility::windows::WStringToString(msg));
		#else
			throw std::runtime_error(msg);
		#endif // defined(_UNICODE) || defined(UNICODE)

		}

	}

	
	void InitializeD3D12Device();

	Microsoft::WRL::ComPtr<ID3D12Device10> const& D3D12Device() noexcept;

}

#endif // !D3D12_CORE_H
