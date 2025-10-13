module;
#ifdef WIN32
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <comdef.h>
#endif // WIN32

module d3d12_backend:util;

namespace fyuu_engine::windows::d3d12 {
#ifdef WIN32
	void ThrowIfFailed(HRESULT result) {
		if (FAILED(result)) {
			throw _com_error(result);
		}
	}
#endif // WIN32

}