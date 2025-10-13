module simple_logger:sink;
import defer;
import std;

namespace fyuu_engine::logger::simple_logger {

	class DefaultFormatter : public IFormatter {
	private:
		static std::string_view LevelToString(LogLevel level) noexcept {
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

	};

	static DefaultFormatter s_default_formatter;

	/*
	*	ConsoleSink
	*/

	void ConsoleSink::ApplyColor(std::ostream& stream, LogLevel level) {
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

	void ConsoleSink::Consume(LogLevel level, std::string_view formatted) {

		auto lock = util::Lock(m_stream_mutex);

		auto& stream = (level >= LogLevel::Warning) ? std::cerr : std::cout;

		ConsoleSink::ApplyColor(stream, level);
		stream << formatted << std::endl
			// reset color
			<< "\033[0m";

	}

	void ConsoleSink::Flush() {

		auto lock = util::Lock(m_stream_mutex);

		std::cout.flush();
		std::cerr.flush();

	}

	void ConsoleSink::Rotate() {

	}

	bool ConsoleSink::ShouldLog(LogLevel level) const noexcept {
		return level >= GetLevel();
	}

	ConsoleSink::ConsoleSink()
		: m_formatter(util::MakeReferred(&s_default_formatter)),
		m_log_level(
#ifdef NDEBUG
			static_cast<std::uint8_t>(LogLevel::Info)
#else
			static_cast<std::uint8_t>(LogLevel::Trace)
#endif // NDEBUG
		) {

	}

	void ConsoleSink::SetLevel(LogLevel level) noexcept {
		m_log_level.store(static_cast<std::uint8_t>(level), std::memory_order::relaxed);
	}

	LogLevel ConsoleSink::GetLevel() const noexcept {
		return static_cast<LogLevel>(m_log_level.load(std::memory_order::relaxed));
	}

	void ConsoleSink::Log(LogEntity const& entity) {

		if (ShouldLog(entity.level)) {
			Consume(entity.level, m_formatter->Format(entity));
		}

	}

	/*
	*	FileSink
	*/

	void FileSink::CleanOldFiles() {
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

	FileSink::FileSink(std::filesystem::path const& file_path, std::size_t max_size, std::size_t max_files)
		: m_formatter(&s_default_formatter),
		m_file_path(file_path),
		m_max_size(max_size),
		m_max_files(max_files),
		m_current_size(0) {

		auto parent_path = m_file_path.parent_path();
		if (!parent_path.empty()) {
			std::filesystem::create_directories(parent_path);
		}

		Rotate();

	}

	FileSink::FileSink(std::filesystem::path const& file_path, FormatterPtr const& formatter, std::size_t max_size, std::size_t max_files)
		: m_formatter(util::MakeReferred(formatter)),
		m_file_path(file_path),
		m_max_size(max_size),
		m_max_files(max_files),
		m_current_size(0) {

		auto parent_path = m_file_path.parent_path();
		if (!parent_path.empty()) {
			std::filesystem::create_directories(parent_path);
		}

		Rotate();

	}
	
	void FileSink::Rotate() {

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

	void FileSink::Consume(LogLevel level, std::string_view formatted) {

		if (m_current_size + formatted.size() > m_max_size) {
			Rotate();
		}

		auto lock = util::Lock(m_stream_mutex);

		if (m_file.is_open()) {
			m_file << formatted << std::endl;
			m_current_size += formatted.size();
		}
	}

	void FileSink::Flush() {
		auto lock = util::Lock(m_stream_mutex);
		if (m_file.is_open()) {
			m_file.flush();
		}
	}

	void FileSink::Log(LogEntity const& entity) {

		if (ShouldLog(entity.level)) {
			Consume(entity.level, m_formatter->Format(entity));
		}

	}

	bool FileSink::ShouldLog(LogLevel level) const noexcept {
		return level >= GetLevel();
	}

	FileSink::~FileSink() noexcept {
		if (m_file.is_open()) {
			m_file.flush();
			m_file.close();
		}
	}

	void FileSink::SetLevel(LogLevel level) noexcept {
		m_log_level.store(static_cast<std::uint8_t>(level),
			std::memory_order::relaxed);
	}

	LogLevel FileSink::GetLevel() const noexcept {
		return static_cast<LogLevel>(
			m_log_level.load(std::memory_order::relaxed));
	}


}