export module logger;
export import pointer_wrapper;
import std;

export namespace core {

	enum class LogLevel : std::uint8_t {
		TRACE,
		DEBUG,
		INFO,
		WARNING,
		ERROR,
		FATAL
	};

	class ILogger {
	protected:
		virtual void LogImp(
			LogLevel level,
			std::source_location const& loc,
			std::string_view message
		) = 0;

		virtual bool ShouldLog(LogLevel level) const noexcept = 0;

	public:
		virtual ~ILogger() = default;

		virtual void SetLevel(LogLevel level) noexcept = 0;
		virtual LogLevel GetLevel() const noexcept = 0;

		template <class... Args>
		void Log(
			LogLevel level,
			std::source_location const& loc,
			std::format_string<Args...> fmt,
			Args&&... args
		) {
			if (!ShouldLog(level)) {
				return;
			}
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
			Log(LogLevel::TRACE, loc, fmt, std::forward<Args>(args)...);
		}

		template <class... Args>
		void Debug(
			std::source_location const& loc,
			std::format_string<Args...> fmt,
			Args&&... args
		) {
			Log(LogLevel::DEBUG, loc, fmt, std::forward<Args>(args)...);
		}

		template <class... Args>
		void Info(
			std::source_location const& loc,
			std::format_string<Args...> fmt,
			Args&&... args
		) {
			Log(LogLevel::INFO, loc, fmt, std::forward<Args>(args)...);
		}

		template <class... Args>
		void Warning(
			std::source_location const& loc,
			std::format_string<Args...> fmt,
			Args&&... args
		) {
			Log(LogLevel::WARNING, loc, fmt, std::forward<Args>(args)...);
		}

		template <class... Args>
		void Error(
			std::source_location const& loc,
			std::format_string<Args...> fmt,
			Args&&... args
		) {
			Log(LogLevel::ERROR, loc, fmt, std::forward<Args>(args)...);
		}

		template <class... Args>
		void Fatal(
			std::source_location const& loc,
			std::format_string<Args...> fmt,
			Args&&... args
		) {
			Log(LogLevel::FATAL, loc, fmt, std::forward<Args>(args)...);
		}

	};

	using LoggerPtr = util::PointerWrapper<ILogger>;

}