module;
#ifdef WIN32
#include <Windows.h>
#include <comdef.h>
#include <boost/system/system_error.hpp>
#endif // WIN32

module fyuu_rhi:d3d12_common;

namespace fyuu_rhi::d3d12 {
#ifdef WIN32

	void ThrowIfFailed(HRESULT result) {
		if (FAILED(result)) {
			throw _com_error(result);
		}
	}

	EventHandle CreateEventHandle() {
		if (HANDLE event_handle = CreateEvent(nullptr, false, false, nullptr)) {
			return EventHandle(event_handle, CloseHandle);
		}
		throw boost::system::system_error(
			GetLastError(),
			boost::system::system_category()
		);
	}

#endif // WIN32

}