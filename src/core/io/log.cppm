module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <exception>
#include <memory>
#include <array>
#include <string_view>
#include <filesystem>
#include <format>
#include <source_location>
#include <print>
#endif // !defined(__cpp_lib_modules)
#include <spdlog/spdlog.h>
#include <spdlog/async.h> 
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "fyuu_log.h"
export module fyuu_engine:log;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

namespace fs = std::filesystem;

namespace fyuu_engine::log {

	export void Initialize() noexcept {
		try	{

			fs::create_directories("logs");
			spdlog::init_thread_pool(8192, 1);

			auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
			
			auto engine_rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/engine.log", 1024 * 1024 * 5, 3);
			auto app_rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/app.log", 1024 * 1024 * 5, 3);

			std::array file_sinks = { engine_rotating_sink, app_rotating_sink };
			for (auto& sink : file_sinks) {
				sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
			}

			auto engine_logger = std::make_shared<spdlog::logger>("Engine", spdlog::sinks_init_list({ console_sink, engine_rotating_sink }));
			auto app_logger = std::make_shared<spdlog::logger>("App", spdlog::sinks_init_list({ console_sink, app_rotating_sink }));

			spdlog::register_logger(engine_logger);
			spdlog::register_logger(app_logger);

#if defined(NDEBUG)
			engine_logger->set_level(spdlog::level::info);
			app_logger->set_level(spdlog::level::info);
#else
			engine_logger->set_level(spdlog::level::debug);
			app_logger->set_level(spdlog::level::debug);
#endif // defined(NDEBUG)
			engine_logger->flush_on(spdlog::level::err);
			app_logger->flush_on(spdlog::level::err);

		}
		catch (spdlog::spdlog_ex const& ex) {
			std::println("log::Initialize() error occurred: {}", ex.what());
		}
		catch (std::exception const& ex) {
			std::println("log::Initialize() error occurred: {}", ex.what());
		}
		catch (...) {
			std::println("log::Initialize() error occurred: unknown exception");
		}
	}

	export void Shutdown() noexcept {
		spdlog::shutdown();
	}

	export void Trace(std::string_view msg, std::source_location const& loc = std::source_location::current()) noexcept {
		auto engine_logger = spdlog::get("Engine");
		if (engine_logger) {
			engine_logger->trace("file: {}({},{}) '{}': {}", loc.file_name(), loc.line(), loc.column(), loc.function_name(), msg);
		}
	}

	export void Debug(std::string_view msg, std::source_location const& loc = std::source_location::current()) noexcept {
		auto engine_logger = spdlog::get("Engine");
		if (engine_logger) {
			engine_logger->debug("file: {}({},{}) '{}': {}", loc.file_name(), loc.line(), loc.column(), loc.function_name(), msg);
		}
	}

	export void Info(std::string_view msg, std::source_location const& loc = std::source_location::current()) noexcept {
		auto engine_logger = spdlog::get("Engine");
		if (engine_logger) {
			engine_logger->info("file: {}({},{}) '{}': {}", loc.file_name(), loc.line(), loc.column(), loc.function_name(), msg);
		}
	}

	export void Warning(std::string_view msg, std::source_location const& loc = std::source_location::current()) noexcept {
		auto engine_logger = spdlog::get("Engine");
		if (engine_logger) {
			engine_logger->warn("file: {}({},{}) '{}': {}", loc.file_name(), loc.line(), loc.column(), loc.function_name(), msg);
		}
	}

	export void Error(std::string_view msg, std::source_location const& loc = std::source_location::current()) noexcept {
		auto engine_logger = spdlog::get("Engine");
		if (engine_logger) {
			engine_logger->error("file: {}({},{}) '{}': {}", loc.file_name(), loc.line(), loc.column(), loc.function_name(), msg);
		}
	}

	export void Fatal(std::string_view msg, std::source_location const& loc = std::source_location::current()) noexcept {
		auto engine_logger = spdlog::get("Engine");
		if (engine_logger) {
			engine_logger->critical("file: {}({},{}) '{}': {}", loc.file_name(), loc.line(), loc.column(), loc.function_name(), msg);
		}
	}

}

extern "C" {
	
	LIB_API void LIB_CALL Fyuu_Trace(char const* msg, char const* file, uint_least32_t line, char const* function) {
		auto app_logger = spdlog::get("App");
		if (app_logger) {
			app_logger->trace("file: {}({}) '{}': {}", file, line, function, msg);
		}
	}
	
	LIB_API void LIB_CALL Fyuu_Debug(char const* msg, char const* file, uint_least32_t line, char const* function) {
		auto app_logger = spdlog::get("App");
		if (app_logger) {
			app_logger->debug("file: {}({}) '{}': {}", file, line, function, msg);
		}
	}
	
	LIB_API void LIB_CALL Fyuu_Info(char const* msg, char const* file, uint_least32_t line, char const* function) {
		auto app_logger = spdlog::get("App");
		if (app_logger) {
			app_logger->info("file: {}({}) '{}': {}", file, line, function, msg);
		}
	}
	
	LIB_API void LIB_CALL Fyuu_Warning(char const* msg, char const* file, uint_least32_t line, char const* function) {
		auto app_logger = spdlog::get("App");
		if (app_logger) {
			app_logger->warn("file: {}({}) '{}': {}", file, line, function, msg);
		}
	}
	
	LIB_API void LIB_CALL Fyuu_Error(char const* msg, char const* file, uint_least32_t line, char const* function) {
		auto app_logger = spdlog::get("App");
		if (app_logger) {
			app_logger->error("file: {}({}) '{}': {}", file, line, function, msg);
		}
	}
	
	LIB_API void LIB_CALL Fyuu_Fatal(char const* msg, char const* file, uint_least32_t line, char const* function) {
		auto app_logger = spdlog::get("App");
		if (app_logger) {
			app_logger->critical("file: {}({}) '{}': {}", file, line, function, msg);
		}
	}
	
}