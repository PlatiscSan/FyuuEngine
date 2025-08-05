export module simple_logger:sink;
import :interface;
import :formatter;
import std;

namespace logger::simple_logger {

	extern Formatter g_default_formatter;

	export class ConsoleSink : public BaseSink {
	private:
		std::atomic_uint8_t m_log_level;

		static void ApplyColor(std::ostream& stream, core::LogLevel level);

		void Consume(core::LogLevel level, std::string_view formatted) override;

		void FlushImp() override;

		void RotateImp() override;

		bool ShouldLog(core::LogLevel level) const noexcept;

	public:
		ConsoleSink();

		void SetLevel(core::LogLevel level) noexcept;

		core::LogLevel GetLevel() const noexcept;

	};

	export class FileSink : public BaseSink {
	private:
		std::ofstream m_file;
		std::filesystem::path m_file_path;
		std::size_t m_max_size;
		std::size_t m_max_files;
		std::size_t m_current_size;
		std::atomic_uint8_t m_log_level;

	private:
		void CleanOldFiles();

		void RotateImp() override;

		void Consume(core::LogLevel level, std::string_view formatted) override;

		void FlushImp() override;

		bool ShouldLog(core::LogLevel level) const noexcept override;

	public:
		template <std::convertible_to<std::filesystem::path> Path>
		FileSink(
			Path&& file_path,
			std::size_t max_size = 10u << 20,
			std::size_t max_files = 10u
		) : BaseSink(&g_default_formatter),
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

		template <std::convertible_to<FormatterPtr> Formatter>
		FileSink(
			std::filesystem::path const& file_path,
			Formatter&& formatter,
			std::size_t max_size = 10 << 20,
			std::size_t max_files = 10
		) : BaseSink(std::forward<Formatter>(formatter)),
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

		~FileSink() noexcept override;

		void SetLevel(core::LogLevel level) noexcept override;

		core::LogLevel GetLevel() const noexcept override;

	};
}