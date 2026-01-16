module;
#include "fyuu_graphics.h"

module fyuu_engine.rhi_error;

static thread_local std::array<std::byte, 1024u> buffer;
static thread_local std::pmr::monotonic_buffer_resource pool{
	std::data(buffer),
	sizeof(buffer),
	std::pmr::null_memory_resource()
};
static thread_local std::pmr::string s_last_error{ &pool };

EXTERN_C DLL_API char const* DLL_CALL FyuuGetLastRHIErrorMessage() NO_EXCEPT {
	return s_last_error.data();
}

namespace fyuu_engine::rhi {

	void SetLastRHIErrorMessage(std::string_view error_msg) {
		s_last_error = error_msg;
	}

}
