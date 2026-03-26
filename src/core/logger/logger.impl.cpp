/* logger.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string>
#include <filesystem>
#include <string_view>
#include <source_location>
#endif
module fyuu_engine:logger_impl;
#if defined(__cpp_lib_modules)
import std;
#endif
import :logger;
import :logger_types;
import :spdlog;

namespace fs = std::filesystem;

namespace fyuu_engine::logger {

	Logger::Logger(
		LoggerBackend backend,
		std::string logger_name,
		fs::path log_path,
		std::size_t max_size,
		std::size_t max_files
	) : m_impl(
		[backend, &logger_name, &log_path, max_size, max_files]() -> plastic::utility::UniqueBase<PolymorphicLoggerBase> {
			switch (backend) {
			case LoggerBackend::PlatformDefault:
			case LoggerBackend::SPDLog:
				return plastic::utility::MakeUnique<spdlog::SPDLog>(logger_name, log_path, max_size, max_files);
			default:
				return nullptr;
			}
		}()) {
	}

	void Logger::Flush() {
		m_impl->Visit(
			[](auto* derived) {
				derived->Flush();
			}
		);
	}

	void Logger::Log(
		LogSeverity severity,
		std::string_view message,
		std::source_location const& location
	) {
		m_impl->Visit(
			[severity, message, &location](auto* derived) {
				derived->Log(severity, message, location);
			}
		);
	}

	void Logger::Info(std::string_view message, std::source_location const& location) {
		Log(LogSeverity::Info, message, location);
	}

	void Logger::Warning(std::string_view message, std::source_location const& location) {
		Log(LogSeverity::Warning, message, location);
	}

	void Logger::Error(std::string_view message, std::source_location const& location) {
		Log(LogSeverity::Error, message, location);
	}

	void Logger::Fatal(std::string_view message, std::source_location const& location) {
		Log(LogSeverity::Fatal, message, location);
	}

}