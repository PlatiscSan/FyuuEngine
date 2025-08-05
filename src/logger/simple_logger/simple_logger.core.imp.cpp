module simple_logger:core;

namespace logger::simple_logger {

	void LoggingCore::IOWorker(std::stop_token const& token) {

		ConsoleSink console;
		SinksMap sinks;
		LogQueue logs;
		std::binary_semaphore semaphore(0);
		m_sinks = &sinks;
		m_queue = &logs;
		sinks.try_emplace("console", &console);

		do {

			if (auto record = logs.TryPopFront()) {
				if (auto sink = sinks.find(record->sink)) {
					(*sink)->Log(*record);
				}
				if (record->to_console) {
					console.Log(*record);
				}
			}
			else {
				m_semaphore.store(&semaphore, std::memory_order::release);
				semaphore.acquire();
				m_semaphore.store(nullptr, std::memory_order::release);
			}

		} while (!token.stop_requested());

		do {
			if (auto record = logs.TryPopFront()) {
				if (auto sink = sinks.find(record->sink)) {
					(*sink)->Log(*record);
				}
				if (record->to_console) {
					console.Log(*record);
				}
			}
		} while (!logs.empty());

		auto modifier = sinks.LockedModifier();
		for (auto& [_, sink] : modifier) {
			sink->Flush();
		}

	}

	void LoggingCore::Notify() {
		std::binary_semaphore* semaphore = m_semaphore.exchange(nullptr, std::memory_order::acq_rel);
		if (semaphore) {
			semaphore->release();
		}
	}

	LoggingCore::LoggingCore() {

		std::jthread io_thread(
			[this](std::stop_token const& token) {
				IOWorker(token);
			}
		);
		io_thread.detach();

		std::binary_semaphore* semaphore = nullptr;
		do {
			semaphore = m_semaphore.load(std::memory_order::acquire);
			if (!semaphore) {
				std::this_thread::yield();
			}
		} while (!semaphore);

	}

	void LoggingCore::Logger::SetLevel(core::LogLevel level) noexcept {
		m_log_level.store(static_cast<std::uint8_t>(level),
			std::memory_order::relaxed);
	}

	core::LogLevel LoggingCore::Logger::GetLevel() const noexcept {
		return static_cast<core::LogLevel>(
			m_log_level.load(std::memory_order::relaxed));
	}

	bool LoggingCore::Logger::ShouldLog(core::LogLevel level) const noexcept {
		return level >= GetLevel();
	}

	LoggingCore::~LoggingCore() noexcept {
		Notify();
	}

	void LoggingCore::RemoveAllSinks() {
		m_sinks->clear();
	}

	void LoggingCore::Logger::LogImp(
		core::LogLevel level,
		std::source_location const& loc,
		std::string_view message
	) {
		LogEntity log;
		log.location = loc;
		log.message = message;
		log.level = level;
		log.sink = m_sink_identity;
		log.to_console = m_to_console;
		auto instance = LoggingCore::Instance();
		instance->m_queue->emplace_back(std::move(log));
		instance->Notify();
	}

}