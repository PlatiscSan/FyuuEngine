export module simple_logger:core;
export import singleton;
import std;
import concurrent_hash_map;
import circular_buffer;
import :interface;
import :sink;

namespace logger::simple_logger {

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
		class Logger : public core::ILogger {
		private:
			std::string m_sink_identity;
			bool m_to_console;
			std::atomic_uint8_t m_log_level;

		public:
			template <std::convertible_to<std::string> SinkIdentity>
			Logger(SinkIdentity&& sink_identity, bool to_console = true)
				: m_sink_identity(std::forward<SinkIdentity>(sink_identity)),
				m_to_console(to_console),
				m_log_level(
#ifdef NDEBUG
					static_cast<std::uint8_t>(core::LogLevel::INFO)
#else
					static_cast<std::uint8_t>(core::LogLevel::TRACE)
#endif // NDEBUG
				) {

			}

			void SetLevel(core::LogLevel level) noexcept override;

			core::LogLevel GetLevel() const noexcept override;

		private:
			bool ShouldLog(core::LogLevel level) const noexcept override;

			void LogImp(
				core::LogLevel level,
				std::source_location const& loc,
				std::string_view message
			);

		};

		~LoggingCore() noexcept;

		template <std::convertible_to<std::string> Identity, std::convertible_to<SinkPtr> Sink>
		bool RegisterSink(Identity&& identity, Sink&& sink) {
			return m_sinks->try_emplace(std::forward<Identity>(identity), std::forward<Sink>(sink)).second;
		}

		template <std::convertible_to<std::string> Identity>
		bool UnregisterSink(Identity&& identity) {
			return m_sinks->erase(std::forward<Identity>(identity));
		}

		void RemoveAllSinks();

	};

	export using Logger = typename LoggingCore::Logger;

}