/* spdlog.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <atomic>
#include <string>
#include <filesystem>
#include <string_view>
#include <source_location>
#endif
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
module fyuu_engine:spdlog_impl;
#if defined(__cpp_lib_modules)
import std;
#endif
import :spdlog;
import :logger_types;

namespace fs = std::filesystem;

namespace {
    std::atomic_size_t s_spdlog_ref;
}

namespace fyuu_engine::logger::spdlog {

	SPDLog::SPDLog(std::string const& name, fs::path const& log_path, std::size_t max_size, std::size_t max_files)
		: PolymorphicLoggerBase(this),
		m_sink(
            [&log_path, max_size, max_files]() {
                if (s_spdlog_ref.fetch_add(1u, std::memory_order::relaxed) == 0u) {
                    ::spdlog::init_thread_pool(8192, 1);
                }
               return std::make_shared<::spdlog::sinks::rotating_file_sink_mt>(log_path.string(), max_size, max_files);
            } ()),
		m_logger(std::make_shared<::spdlog::async_logger>(name, m_sink, ::spdlog::thread_pool(), ::spdlog::async_overflow_policy::block)) {

	}

	SPDLog::~SPDLog() noexcept {
		Flush();
        if (s_spdlog_ref.fetch_sub(1u, std::memory_order::relaxed) == 1u) {
			::spdlog::drop_all();
			::spdlog::shutdown();
		}
	}

	void SPDLog::Log(
		LogSeverity severity,
		std::string_view message,
		std::source_location const& location
	) {
		::spdlog::level::level_enum level;
		switch (severity) {
		case LogSeverity::Info:    level = ::spdlog::level::info; break;
		case LogSeverity::Warning: level = ::spdlog::level::warn; break;
		case LogSeverity::Error:   level = ::spdlog::level::err; break;
		case LogSeverity::Fatal:   level = ::spdlog::level::critical; break;
		default: level = ::spdlog::level::info;
		}

		::spdlog::source_loc spdlog_location{
			location.file_name(),
			static_cast<int>(location.line()),
			location.function_name()
		};

		m_logger->log(spdlog_location, level, message);
		::spdlog::log(spdlog_location, level, message);

	}

	void SPDLog::Flush() {
		m_logger->flush();
	}

}