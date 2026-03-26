/* spdlog.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string>
#include <filesystem>
#include <string_view>
#include <source_location>
#endif
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
export module fyuu_engine:spdlog;
#if defined(__cpp_lib_modules)
import std;
#endif
import :logger_types;

namespace fs = std::filesystem;

namespace fyuu_engine::logger::spdlog {

	export class SPDLog
		: public PolymorphicLoggerBase {
	private:
		::spdlog::sink_ptr m_sink;
		std::shared_ptr<::spdlog::async_logger> m_logger;

	public:
		SPDLog(std::string const& name, fs::path const& log_path, std::size_t max_size, std::size_t max_files);
		~SPDLog() noexcept;

		void Log(
			LogSeverity severity,
			std::string_view message,
			std::source_location const& location = std::source_location::current()
		);

		void Flush();

	};
	
}
