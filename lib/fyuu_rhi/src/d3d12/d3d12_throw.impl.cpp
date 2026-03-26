/* d3d12_throw.cpp */
module;
#include "platform.hpp"
module fyuu_rhi:d3d12_throw;
#if defined(_WIN32)
namespace fyuu_rhi::d3d12 {
	void ThrowIfFailed(HRESULT result) {
		if (FAILED(result)) {
			throw _com_error(result);
		}
	}
}
#endif // defined(_WIN32)
