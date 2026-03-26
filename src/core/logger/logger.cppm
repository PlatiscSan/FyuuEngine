/* logger.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string>
#include <filesystem>
#include <string_view>
#include <source_location>
#endif
export module fyuu_engine:logger;
#if defined(__cpp_lib_modules)
import std;
#endif
import plastic.sealed_polymorphism;
import :logger_types;

namespace fs = std::filesystem;

namespace fyuu_engine::logger {

	export class Logger {
	private:
		plastic::utility::UniqueBase<PolymorphicLoggerBase> m_impl;

	public:
		Logger(
			LoggerBackend backend,
			std::string logger_name,
			fs::path log_path,
			std::size_t max_size = 10 * 1024 * 1024,	// 10MB for default
			std::size_t max_files = 3
		);

		void Flush();

		void Log(LogSeverity severity, std::string_view message, std::source_location const& location = std::source_location::current());

		void Info(std::string_view message, std::source_location const& location = std::source_location::current());

		void Warning(std::string_view message, std::source_location const& location = std::source_location::current());

		void Error(std::string_view message, std::source_location const& location = std::source_location::current());

		void Fatal(std::string_view message, std::source_location const& location = std::source_location::current());
		
	};

}