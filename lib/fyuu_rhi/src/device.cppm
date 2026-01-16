export module fyuu_rhi:device;
import std;
import :swap_chain;
import :command_object;
import :pipeline;
import :memory;
import plastic.disable_copy;

namespace fyuu_rhi {

	export enum class LogSeverity : std::uint8_t {
		Info = 0,
		Warning,
		Error,
		Fatal
	};

	export using LogCallback = void(*)(LogSeverity, std::string);

	export struct RHIInitOptions {
		/// @brief Name of app.
		std::string_view app_name;

		/// @brief Name of engine.
		std::string_view engine_name;

		struct Version {
			std::uint8_t variant = 0;
			std::uint8_t major = 0;
			std::uint8_t minor = 0;
			std::uint8_t patch = 0;
		};

		/// @brief The version number of app.
		Version app_version;

		/// @brief The version number of engine.
		Version engine_version;

		LogCallback log_callback = nullptr;

		bool software_fallback = false;
	};

	export class IPhysicalDevice 
		: public plastic::utility::DisableCopy<IPhysicalDevice> {
	public:

	};

	export class ILogicalDevice 
		: public plastic::utility::DisableCopy<ILogicalDevice> {
	public:

	};


}