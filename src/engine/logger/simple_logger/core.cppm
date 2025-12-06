export module logger:simple_logger_core;
export import singleton;
import std;
import circular_buffer;
import :interface;
import :simple_logger_sink;

namespace fyuu_engine::logger::simple_logger {

	export template <SinkConcept FileSink, SinkConcept ConsoleSink> class LoggingCore 
		: public BaseCore<LoggingCore<FileSink, ConsoleSink>>, public util::Singleton<LoggingCore<FileSink, ConsoleSink>> {
	private:
		friend class util::Singleton<LoggingCore>;
		friend class BaseCore<LoggingCore<FileSink, ConsoleSink>>;

		struct SinksMap {
			std::unordered_map<SinkID, FileSink*> impl;
			std::mutex impl_mutex;
		};
		using LogQueue = concurrency::CircularBuffer<LogEntity, 64u>;

		std::jthread m_io_thread;
		SinksMap* m_sinks = nullptr;
		LogQueue* m_queue = nullptr;
		std::atomic<std::binary_semaphore*> m_semaphore = nullptr;

		void IOWorker(std::stop_token const& token) {

			ConsoleSink console{};
			SinksMap sinks{};
			LogQueue logs;
			std::binary_semaphore semaphore(0);
			m_sinks = &sinks;
			m_queue = &logs;

			do {

				if (std::optional<LogEntity> entity = logs.TryPopFront()) {
					std::lock_guard<std::mutex> lock(sinks.impl_mutex);
					console.Log(*entity);
					FileSink* sink = sinks.impl[entity->sink];
					if (!sink) {
						continue;
					}
					sink->Log(*entity);
				}
				else {
					m_semaphore.store(&semaphore, std::memory_order::release);
					semaphore.acquire();
					m_semaphore.store(nullptr, std::memory_order::release);
				}

			} while (!token.stop_requested());

			{
				/*
				*	clean up before quit
				*/

				std::lock_guard<std::mutex> lock(sinks.impl_mutex);

				do {
					if (std::optional<LogEntity> entity = logs.TryPopFront()) {
						console.Log(*entity);
						FileSink* sink = sinks.impl[entity->sink];
						if (!sink) {
							continue;
						}
						sink->Log(*entity);
					}
				} while (!logs.empty());

				for (auto& [_, sink] : sinks.impl) {
					sink->Flush();
				}

			}
		}

		void Notify() {
			std::binary_semaphore* semaphore = m_semaphore.exchange(nullptr, std::memory_order::acq_rel);
			if (semaphore) {
				semaphore->release();
			}
		}

		LoggingCore()
			: m_io_thread(
				[this](std::stop_token const& token) {
					IOWorker(token);
				}
			) {

			std::binary_semaphore* semaphore = nullptr;
			do {
				/*
				*	wait for io worker initialization
				*/

				semaphore = m_semaphore.load(std::memory_order::acquire);
				if (!semaphore) {
					std::this_thread::yield();
				}
			} while (!semaphore);

		}

		template <SinkConcept Sink>
		void RegisterSinkImpl(Sink&& sink) {
			std::lock_guard<std::mutex> lock(m_sinks->impl_mutex);
			m_sinks->impl[sink.GetID()] = &sink;
		}

	public:
		class Logger : public BaseLogger<Logger> {
		private:
			SinkID m_sink;
			bool m_to_console;
			std::atomic_uint8_t m_log_level;

			bool ShouldLog(LogLevel level) const noexcept {
				return level >= GetLevelImpl();
			}

			void SetLevelImpl(LogLevel level) noexcept {
				m_log_level.store(static_cast<std::uint8_t>(level),
					std::memory_order::relaxed);
			}

			LogLevel GetLevelImpl() const noexcept {
				return static_cast<LogLevel>(
					m_log_level.load(std::memory_order::relaxed));
			}

			void LogImp(
				LogLevel level,
				std::string_view message,
				std::source_location const& loc
			) const {

				if (!ShouldLog(level)) {
					return;
				}

				LogEntity log;
				log.location = loc;
				log.message = message;
				log.level = level;
				log.sink = m_sink;
				log.to_console = m_to_console;
				LoggingCore* instance = LoggingCore::Instance();
				instance->m_queue->emplace_back(std::move(log));
				instance->Notify();
			}

		public:
			Logger(SinkID sink, bool to_console) noexcept
				: m_sink(sink), m_to_console(to_console) {
			}

		};

		~LoggingCore() noexcept {
			m_io_thread.request_stop();
			Notify();
		}

		void RemoveSink(SinkID sink) {
			std::lock_guard<std::mutex> lock(m_sinks->impl_mutex);
			m_sinks->impl.erase(sink);
		}

	};

}