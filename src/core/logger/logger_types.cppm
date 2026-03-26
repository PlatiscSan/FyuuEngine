/* logger types */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <ctime>
#endif
export module fyuu_engine:logger_types;
#if defined(__cpp_lib_modules)
import std;
#endif
import plastic.sealed_polymorphism;

namespace fyuu_engine::logger {

	export enum class LogSeverity : std::uint8_t {
		Info = 0,
		Warning,
		Error,
		Fatal
	};

	namespace spdlog {
		export class SPDLog;
	}

	using PolymorphicLoggerBase = plastic::utility::PolymorphicBase<
		std::size_t,
		spdlog::SPDLog
	>;

	enum class LoggerBackend : std::uint8_t {
		PlatformDefault,
		SPDLog,
		BoostLog
	};

}