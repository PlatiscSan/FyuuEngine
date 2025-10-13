#ifndef FYUU_LOGGER_H
#define FYUU_LOGGER_H

#include <export_macros.h>

#ifdef __cplusplus
#include <cstdint>
#include <cstdarg>
#include <source_location>
#include <string_view>
#include <format>
#else
#include <stdint.h>
#include <stdarg.h>
#endif

#ifdef __cplusplus
EXTERN_C{
#endif // __cplusplus
	typedef enum Fyuu_LogLevel {
		FYUU_LOG_LEVEL_TRACE = 0,
		FYUU_LOG_LEVEL_DEBUG,
		FYUU_LOG_LEVEL_INFO,
		FYUU_LOG_LEVEL_WARNING,
		FYUU_LOG_LEVEL_ERROR,
		FYUU_LOG_LEVEL_FATAL
	} FyuuLogLevel;

	typedef struct Fyuu_SourceLocation {
		char const* file_name;
		char const* function_name;
		uint_least32_t line;
		uint_least32_t column;
	} FyuuSourceLocation;

	typedef struct Fyuu_ILogger {
	

	} Fyuu_ILogger;


#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef __cplusplus
namespace fyuu_engine::application {

	enum class LogLevel : std::uint8_t {
		Trace,
		Debug,
		Info,
		Warning,
		Error,
		Fatal
	};

	class DLL_API ILogger {
	protected:
		virtual void LogImp(
			LogLevel level,
			std::source_location const& loc,
			std::string_view message
		) = 0;

	public:
		virtual ~ILogger() = default;

		template <class... Args>
		void Log(
			LogLevel level,
			std::source_location const& loc,
			std::format_string<Args...> fmt,
			Args&&... args
		) {
			std::tuple<Args...> args_tuple(std::forward<Args>(args)...);
			std::apply(
				[this, level, &loc, &fmt](auto&&... args) {
					LogImp(level, loc, std::vformat(fmt.get(), std::make_format_args(args...)));
				},
				args_tuple
			);
		}

		template <class... Args>
		void Trace(
			std::source_location const& loc,
			std::format_string<Args...> fmt,
			Args&&... args
		) {
			Log(LogLevel::Trace, loc, fmt, std::forward<Args>(args)...);
		}

		template <class... Args>
		void Debug(
			std::source_location const& loc,
			std::format_string<Args...> fmt,
			Args&&... args
		) {
			Log(LogLevel::Debug, loc, fmt, std::forward<Args>(args)...);
		}

		template <class... Args>
		void Info(
			std::source_location const& loc,
			std::format_string<Args...> fmt,
			Args&&... args
		) {
			Log(LogLevel::Info, loc, fmt, std::forward<Args>(args)...);
		}

		template <class... Args>
		void Warning(
			std::source_location const& loc,
			std::format_string<Args...> fmt,
			Args&&... args
		) {
			Log(LogLevel::Warning, loc, fmt, std::forward<Args>(args)...);
		}

		template <class... Args>
		void Error(
			std::source_location const& loc,
			std::format_string<Args...> fmt,
			Args&&... args
		) {
			Log(LogLevel::Error, loc, fmt, std::forward<Args>(args)...);
		}

		template <class... Args>
		void Fatal(
			std::source_location const& loc,
			std::format_string<Args...> fmt,
			Args&&... args
		) {
			Log(LogLevel::Fatal, loc, fmt, std::forward<Args>(args)...);
		}

	};

} // namespace fyuu_engine::application

#endif // __cplusplus

#endif // !FYUU_LOGGER_H