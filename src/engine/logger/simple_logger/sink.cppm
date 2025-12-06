export module logger:simple_logger_sink;
export import :interface;
import std;
import defer;

namespace fyuu_engine::logger::simple_logger {

	namespace details {
		template <class T> concept FormatterConcept = requires (T t) {
			{ t(std::declval<LogEntity>()) } -> std::convertible_to<std::string>;
		};
	}

	export class Formatter {
	private:
		static std::string_view LevelToString(LogLevel level) noexcept;
	public:
		std::string operator()(LogEntity const& log);
	};

	export template <details::FormatterConcept Formatter> class ConsoleSink 
		: public BaseSink<ConsoleSink<Formatter>> {
		friend class BaseSink<ConsoleSink<Formatter>>;
	private:
		std::atomic_flag m_stream_mutex;
		Formatter m_formatter;
		std::atomic_uint8_t m_log_level;

		static void ApplyColor(std::ostream& stream, LogLevel level) {
			switch (level) {
			case LogLevel::Trace:
				// gray
				stream << "\033[37m";
				break;
			case LogLevel::Debug:
				// blue
				stream << "\033[36m";
				break;
			case LogLevel::Info:
				// green
				stream << "\033[32m";
				break;
			case LogLevel::Warning:
				// yellow
				stream << "\033[33m";
				break;
			case LogLevel::Error:
				// red
				stream << "\033[31m";
				break;
			case LogLevel::Fatal:
				// purple
				stream << "\033[35m";
				break;
			default:
				break;
			}

		}

		void SetLevelImpl(LogLevel level) noexcept {
			m_log_level.store(static_cast<std::uint8_t>(level), std::memory_order::relaxed);
		}

		LogLevel GetLevelImpl() const noexcept {
			return static_cast<LogLevel>(m_log_level.load(std::memory_order::relaxed));
		}

		void Consume(LogLevel level, std::string const& formatted) {

			auto lock = util::Lock(m_stream_mutex);

			std::ostream& stream = (level >= LogLevel::Warning) ? std::cerr : std::cout;

			ConsoleSink::ApplyColor(stream, level);
			stream << formatted << std::endl
				// reset color
				<< "\033[0m";

		}

		void FlushImpl() {

			auto lock = util::Lock(m_stream_mutex);

			std::cout.flush();
			std::cerr.flush();

		}

		void RotateImpl() noexcept {

		}

		bool ShouldLog(LogLevel level) const noexcept {
			return level >= GetLevelImpl();
		}

		void LogImpl(LogEntity const& entity) {
			if (entity.to_console && ShouldLog(entity.level)) {
				Consume(entity.level, m_formatter(entity));
			}
		}

	public:
		using FormatterType = Formatter;

#ifdef NDEBUG
		ConsoleSink(LogLevel level = LogLevel::Info)
			: m_log_level(static_cast<std::uint8_t>(level)) {

		}
#else
		ConsoleSink(LogLevel level = LogLevel::Debug)
			: m_log_level(static_cast<std::uint8_t>(level)) {

		}
#endif // NDEBUG

	};

	export template <details::FormatterConcept Formatter> class FileSink 
		: public BaseSink<FileSink<Formatter>> {
		friend class BaseSink<FileSink<Formatter>>;

	private:
		std::ofstream m_file;
		std::filesystem::path m_file_path;
		std::size_t m_max_size;
		std::size_t m_max_files;
		std::size_t m_current_size;
		std::atomic_flag m_stream_mutex;
		Formatter m_formatter;
		std::atomic_uint8_t m_log_level;

		void SetLevelImpl(LogLevel level) noexcept {
			m_log_level.store(static_cast<std::uint8_t>(level), std::memory_order::relaxed);
		}

		LogLevel GetLevelImpl() const noexcept {
			return static_cast<LogLevel>(m_log_level.load(std::memory_order::relaxed));
		}

		void CleanOldFiles() {
			
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

		void Consume(LogLevel level, std::string const& formatted) {

			if (m_current_size + formatted.size() > m_max_size) {
				RotateImpl();
			}

			auto lock = util::Lock(m_stream_mutex);

			if (m_file.is_open()) {
				m_file << formatted << std::endl;
				m_current_size += formatted.size();
			}

		}

		bool ShouldLog(LogLevel level) const noexcept {
			return level >= GetLevelImpl();
		}


		void RotateImpl() {

			auto lock = util::Lock(m_stream_mutex);

			if (m_file.is_open()) {
				m_file.flush();
				m_file.close();
			}

			auto parent_path = m_file_path.parent_path();
			auto stem = m_file_path.stem().string();
			auto extension = m_file_path.extension().string();

			auto now = std::chrono::system_clock::now();
			constexpr char const* timestamp_str = "{:%Y%m%d_%H-%M-%S}";

			std::string timestamp = std::format(timestamp_str, now);

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

		void FlushImpl() {
			auto lock = util::Lock(m_stream_mutex);
			if (m_file.is_open()) {
				m_file.flush();
			}
		}

		void LogImpl(LogEntity const& entity) {
			if (ShouldLog(entity.level)) {
				Consume(entity.level, m_formatter(entity));
			}
		}

	public:
		using FormatterType = Formatter;

		FileSink(
			std::filesystem::path const& file_path,
			std::size_t max_size = 10u << 20,
			std::size_t max_files = 10u
		) : m_file_path(file_path),
			m_max_size(max_size),
			m_max_files(max_files),
			m_current_size(0) {

			auto parent_path = m_file_path.parent_path();
			if (!parent_path.empty()) {
				std::filesystem::create_directories(parent_path);
			}

			RotateImpl();

		}

		~FileSink() noexcept {
			if (m_file.is_open()) {
				m_file.flush();
				m_file.close();
			}
		}

	};

}