export module simple_logger;
export import logger_interface;
import std;
import singleton;
import concurrent_hash_map;
import circular_buffer;
import concurrent_vector;
import defer;
import pointer_wrapper;
import synchronized_function;

namespace core::simple_logger {

	struct LogEntity {
		std::thread::id thread_id = std::this_thread::get_id();
		std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
		std::source_location location;
		std::string message;
		LogLevel level;
		std::string sink;
		bool to_console;
	};

}

export namespace core::simple_logger {

	class IFormatter {
	public:
		virtual ~IFormatter() = default;
		virtual std::string Format(LogEntity const& log) const = 0;
	};

	class Formatter : public IFormatter {
	private:
		static std::string_view LevelToString(LogLevel level) {

			static std::array<char const*, 6> level_str = {
				"TRACE",
				"DEBUG",
				"INFO",
				"WARNING",
				"ERROR",
				"FATAL"
			};

			auto level_index = static_cast<std::size_t>(level);

			return level_index >= 6 ? "UNKNOWN" : level_str[level_index];

		}

	public:
		std::string Format(LogEntity const& log) const override {
			return std::format(
				"[{}] [{:%Y-%m-%d %H:%M:%S}] [{}] [{}:{}] - {}",
				log.thread_id,
				log.timestamp,
				LevelToString(log.level),
				log.location.function_name(),
				log.location.line(),
				log.message
			);
		}

		std::string operator()(LogEntity const& log) const {
			return Format(log);
		}

	};

	using FormatterPtr = util::PointerWrapper<Formatter>;

	class ISink {
	private:
		static inline Formatter s_default_formatter;
		FormatterPtr m_formatter;

	protected:
		virtual bool ShouldLog(LogLevel level) const noexcept = 0;
		virtual void Consume(LogLevel level, std::string_view formatted) = 0;
		virtual void FlushImp() = 0;
		virtual void RotateImp() = 0;

	public:
		concurrency::SynchronizedFunction<void(LogEntity)> const Log;
		concurrency::SynchronizedFunction<void()> const Flush;
		concurrency::SynchronizedFunction<void()> const Rotate;

		ISink()
			: m_formatter(&s_default_formatter),
			Log(
				[this](LogEntity const& log) {
					if (ShouldLog(log.level)) {
						Consume(log.level, m_formatter->Format(log));
					}
				}
			), 
			Flush(
				[this]() {
					FlushImp();
				}
			),
			Rotate(
				[this]() {
					RotateImp();
				}
			) {

		}

		template <std::convertible_to<FormatterPtr> Formatter>
		explicit ISink(Formatter&& formatter)
			: m_formatter(std::forward<Formatter>(formatter)),
			Log(
				[this](LogEntity const& log) {
					if (ShouldLog(log.level)) {
						Consume(log.level, m_formatter->Format(log));
					}
				}
			),
			Flush(
				[this]() {
					FlushImp();
				}
			),
			Rotate(
				[this]() {
					RotateImp();
				}
			) {

		}

		virtual ~ISink() noexcept = default;

		virtual void SetLevel(LogLevel level) noexcept = 0;
		virtual LogLevel GetLevel() const noexcept = 0;

	};

	using SinkPtr = util::PointerWrapper<ISink>;

	class ConsoleSink : public ISink {
	private:
		std::atomic_uint8_t m_log_level =
#ifdef NDEBUG
			static_cast<std::uint8_t>(LogLevel::INFO);
#else
			static_cast<std::uint8_t>(LogLevel::TRACE);
#endif // NDEBUG

		static void ApplyColor(std::ostream& stream, LogLevel level) {
			switch (level) {
			case LogLevel::TRACE: 
				// gray
				stream << "\033[37m";
				break;
			case LogLevel::DEBUG: 
				// blue
				stream << "\033[36m"; 
				break;
			case LogLevel::INFO:  
				// green
				stream << "\033[32m"; 
				break;
			case LogLevel::WARNING:
				// yellow
				stream << "\033[33m"; 
				break;
			case LogLevel::ERROR: 
				// red
				stream << "\033[31m"; 
				break;
			case LogLevel::FATAL: 
				// purple
				stream << "\033[35m";
				break;
			default: 
				break;
			}
		}

		void Consume(LogLevel level, std::string_view formatted) override {

			auto& stream = (level >= LogLevel::WARNING) ? std::cerr : std::cout;

			ConsoleSink::ApplyColor(stream, level);
			stream << formatted << std::endl
				// reset color
				<< "\033[0m";

		}

		void FlushImp() override {
			std::cout.flush();
			std::cerr.flush();
		}

		void RotateImp() override {

		}

	public:
		ConsoleSink()
			: ISink() {

		}

		void SetLevel(LogLevel level) noexcept override {
			m_log_level.store(static_cast<std::uint8_t>(level), std::memory_order::relaxed);
		}

		LogLevel GetLevel() const noexcept override {
			return static_cast<LogLevel>(m_log_level.load(std::memory_order::relaxed));
		}

	private:
		bool ShouldLog(LogLevel level) const noexcept override {
			return level >= GetLevel();
		}

	};

	class FileSink : public ISink {
	private:
		std::ofstream m_file;
		std::filesystem::path m_file_path;
		std::size_t m_max_size;
		std::size_t m_max_files;
		std::size_t m_current_size;
		std::atomic_uint8_t m_log_level =
#ifdef NDEBUG
			static_cast<std::uint8_t>(LogLevel::INFO);
#else
			static_cast<std::uint8_t>(LogLevel::TRACE);
#endif // NDEBUG
	public:
		void SetLevel(LogLevel level) noexcept override {
			m_log_level.store(static_cast<std::uint8_t>(level),
				std::memory_order::relaxed);
		}

		LogLevel GetLevel() const noexcept override {
			return static_cast<LogLevel>(
				m_log_level.load(std::memory_order::relaxed));
		}

	private:
		void CleanOldFiles() {
			try {
				auto parent_path = m_file_path.parent_path();
				auto stem = m_file_path.stem().string();
				auto extension = m_file_path.extension().string();

				std::vector<std::filesystem::path> files;
				for (auto const& entry : std::filesystem::directory_iterator(parent_path)) {
					
					if (!entry.is_regular_file()) {
						continue;
					}

					auto const& path = entry.path();
					auto filename = path.filename().string();
					
					if (filename.starts_with(stem + "_") &&
						filename.ends_with(extension) &&
						filename != m_file_path.filename()) {
						files.push_back(path);
					}

				}

				std::sort(
					files.begin(), files.end(),
					[](auto const& a, auto const& b) {
						return std::filesystem::last_write_time(a) <
							std::filesystem::last_write_time(b);
					}
				);


				if (files.size() > m_max_files) {
					std::size_t count_to_remove = files.size() - m_max_files;
					for (size_t i = 0; i < count_to_remove; ++i) {
						std::filesystem::remove(files[i]);
					}
				}
			}
			catch (...) {
				
			}
		}

		void RotateImp() override {

			if (m_file.is_open()) {
				m_file.flush();
				m_file.close();
			}

			auto parent_path = m_file_path.parent_path();
			auto stem = m_file_path.stem().string();
			auto extension = m_file_path.extension().string();

			auto now = std::chrono::system_clock::now();
			std::string timestamp = std::format("{:%Y%m%d_%H-%M-%S}", now);

			auto new_filename = stem + "_" + timestamp + extension;
			auto new_path = parent_path / new_filename;
			if (std::filesystem::exists(m_file_path)) {
				std::filesystem::rename(m_file_path, new_path);
			}
			
			m_file.open(new_path, std::ios::app);
			m_current_size = 0;
			
			CleanOldFiles();
		
			m_file_path = new_path;

		}

		void Consume(LogLevel level, std::string_view formatted) override {
			if (m_current_size + formatted.size() > m_max_size) {
				Rotate();
			}

			if (m_file.is_open()) {
				m_file << formatted << std::endl;
				m_current_size += formatted.size();
			}
		}

		void FlushImp() override {
			if (m_file.is_open()) {
				m_file.flush();
			}
		}

		bool ShouldLog(LogLevel level) const noexcept override {
			return level >= GetLevel();
		}

	public:
		template <std::convertible_to<std::filesystem::path> Path>
		FileSink(
			Path&& file_path,
			std::size_t max_size = 10u << 20,
			std::size_t max_files = 10u
		) : ISink(),
			m_file_path(std::forward<Path>(file_path)),
			m_max_size(max_size),
			m_max_files(max_files),
			m_current_size(0) {
			
			auto parent_path = m_file_path.parent_path();
			if (!parent_path.empty()) {
				std::filesystem::create_directories(parent_path);
			}

			RotateImp();

		}

		template <std::convertible_to<FormaterPtr> Formater>
		FileSink(
			std::filesystem::path const& file_path,
			Formatter&& formatter,
			std::size_t max_size = 10 << 20,
			std::size_t max_files = 10
		) : ISink(std::forward<Formatter>(formatter)),
			m_file_path(file_path),
			m_max_size(max_size),
			m_max_files(max_files),
			m_current_size(0) {

			auto parent_path = m_file_path.parent_path();
			if (!parent_path.empty()) {
				std::filesystem::create_directories(parent_path);
			}

			RotateImp();

		}

		~FileSink() noexcept override {
			if (m_file.is_open()) {
				m_file.flush();
				m_file.close();
			}
		}

	};

	class LoggingCore : public util::Singleton<LoggingCore> {
	private:
		friend class util::Singleton<LoggingCore>;
		using SinksMap = concurrency::ConcurrentHashMap<std::string, SinkPtr>;
		using LogQueue = concurrency::CircularBuffer<LogEntity, 64u>;

		SinksMap* m_sinks = nullptr;
		LogQueue* m_queue = nullptr;
		std::atomic<std::binary_semaphore*> m_semaphore = nullptr;
		std::jthread m_io_thread;

		void IOWorker(std::stop_token const& token) {

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
				semaphore = m_semaphore.load(std::memory_order::acquire);
				if (!semaphore) {
					std::this_thread::yield();
				}
			} while (!semaphore);

		}

	public:
		class Logger : public ILogger {
		private:
			std::string m_sink_identity;
			bool m_to_console;
			std::atomic_uint8_t m_log_level =
#ifdef NDEBUG
				static_cast<std::uint8_t>(LogLevel::INFO);
#else
				static_cast<std::uint8_t>(LogLevel::TRACE);
#endif // NDEBUG

		public:
			template <std::convertible_to<std::string> SinkIdentity>
			Logger(SinkIdentity&& sink_identity, bool to_console)
				: m_sink_identity(std::forward<SinkIdentity>(sink_identity)),
				m_to_console(to_console) {

			}

			void SetLevel(LogLevel level) noexcept override {
				m_log_level.store(static_cast<std::uint8_t>(level),
					std::memory_order::relaxed);
			}

			LogLevel GetLevel() const noexcept override {
				return static_cast<LogLevel>(
					m_log_level.load(std::memory_order::relaxed));
			}

		private:
			bool ShouldLog(LogLevel level) const noexcept override {
				return level >= GetLevel();
			}

			void LogImp(
				LogLevel level,
				std::source_location const& loc,
				std::string_view message
			) override {
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

		};

		~LoggingCore() noexcept {
			Notify();
		}

		template <std::convertible_to<std::string> Identity, std::convertible_to<SinkPtr> Sink>
		bool RegisterSink(Identity&& identity, Sink&& sink) {
			return m_sinks->try_emplace(std::forward<Identity>(identity), std::forward<Sink>(sink)).second;
		}

		template <std::convertible_to<std::string> Identity>
		bool UnregisterSink(Identity&& identity) {
			return m_sinks->erase(std::forward<Identity>(identity));
		}

		void RemoveAllSinks() {
			m_sinks->clear();
		}

	};

	using Logger = typename LoggingCore::Logger;

}