export module simple_logger:core;
export import singleton;
import std;
import concurrent_hash_map;
import circular_buffer;
import :interface;
import :sink;

namespace fyuu_engine::logger::simple_logger {

	export class LoggingCore : public util::Singleton<LoggingCore> {
	private:
		friend class util::Singleton<LoggingCore>;
		using SinksMap = concurrency::ConcurrentHashMap<std::string, SinkPtr>;
		using LogQueue = concurrency::CircularBuffer<LogEntity, 64u>;

		std::jthread m_io_thread;
		SinksMap* m_sinks = nullptr;
		LogQueue* m_queue = nullptr;
		std::atomic<std::binary_semaphore*> m_semaphore = nullptr;

		void IOWorker(std::stop_token const& token);

		void Notify();

		LoggingCore();

	public:
		class Logger {
		private:
			std::string m_sink_identity;
			bool m_to_console;
			std::atomic_uint8_t m_log_level;

			bool ShouldLog(LogLevel level) const noexcept;

			void LogImp(
				LogLevel level,
				std::source_location const& loc,
				std::string_view message
			) const;

		public:
			Logger(std::string_view sink_identity, bool to_console = true);

			void SetLevel(LogLevel level) noexcept;

			LogLevel GetLevel() const noexcept;

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

		~LoggingCore() noexcept;

		bool RegisterSink(std::string const& identity, SinkPtr const& sink);

		bool UnregisterSink(std::string const& identity);

		void RemoveAllSinks();

	};

	export using Logger = typename LoggingCore::Logger;

}