export module fyuu_engine.rhi_error;
import std;

namespace fyuu_engine::rhi {
	export extern void SetLastRHIErrorMessage(std::string_view message);
}