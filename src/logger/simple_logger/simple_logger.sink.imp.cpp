module simple_logger:sink;

namespace logger::simple_logger {

	Formatter g_default_formatter;

	/*
	*	ConsoleSink
	*/

	void ConsoleSink::ApplyColor(std::ostream& stream, core::LogLevel level) {
		switch (level) {
		case core::LogLevel::TRACE:
			// gray
			stream << "\033[37m";
			break;
		case core::LogLevel::DEBUG:
			// blue
			stream << "\033[36m";
			break;
		case core::LogLevel::INFO:
			// green
			stream << "\033[32m";
			break;
		case core::LogLevel::WARNING:
			// yellow
			stream << "\033[33m";
			break;
		case core::LogLevel::ERROR:
			// red
			stream << "\033[31m";
			break;
		case core::LogLevel::FATAL:
			// purple
			stream << "\033[35m";
			break;
		default:
			break;
		}
	}

	void ConsoleSink::Consume(core::LogLevel level, std::string_view formatted) {

		auto& stream = (level >= core::LogLevel::WARNING) ? std::cerr : std::cout;

		ConsoleSink::ApplyColor(stream, level);
		stream << formatted << std::endl
			// reset color
			<< "\033[0m";

	}

	void ConsoleSink::FlushImp() {
		std::cout.flush();
		std::cerr.flush();
	}

	void ConsoleSink::RotateImp() {

	}

	bool ConsoleSink::ShouldLog(core::LogLevel level) const noexcept {
		return level >= GetLevel();
	}

	ConsoleSink::ConsoleSink()
		: BaseSink(&g_default_formatter),
		m_log_level(
#ifdef NDEBUG
			static_cast<std::uint8_t>(core::LogLevel::INFO)
#else
			static_cast<std::uint8_t>(core::LogLevel::TRACE)
#endif // NDEBUG
		) {

	}

	void ConsoleSink::SetLevel(core::LogLevel level) noexcept {
		m_log_level.store(static_cast<std::uint8_t>(level), std::memory_order::relaxed);
	}

	core::LogLevel ConsoleSink::GetLevel() const noexcept {
		return static_cast<core::LogLevel>(m_log_level.load(std::memory_order::relaxed));
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

	void FileSink::RotateImp() {

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

	void FileSink::Consume(core::LogLevel level, std::string_view formatted) {
		if (m_current_size + formatted.size() > m_max_size) {
			Rotate();
		}

		if (m_file.is_open()) {
			m_file << formatted << std::endl;
			m_current_size += formatted.size();
		}
	}

	void FileSink::FlushImp() {
		if (m_file.is_open()) {
			m_file.flush();
		}
	}

	bool FileSink::ShouldLog(core::LogLevel level) const noexcept {
		return level >= GetLevel();
	}

	FileSink::~FileSink() noexcept {
		if (m_file.is_open()) {
			m_file.flush();
			m_file.close();
		}
	}

	void FileSink::SetLevel(core::LogLevel level) noexcept {
		m_log_level.store(static_cast<std::uint8_t>(level),
			std::memory_order::relaxed);
	}

	core::LogLevel FileSink::GetLevel() const noexcept {
		return static_cast<core::LogLevel>(
			m_log_level.load(std::memory_order::relaxed));
	}


}