export module simple_logger:sink;
export import :interface;
import std;

namespace fyuu_engine::logger::simple_logger {

	export class ConsoleSink : public ISink {
	private:
		FormatterPtr m_formatter;
		std::atomic_flag m_stream_mutex;
		std::atomic_uint8_t m_log_level;

		static void ApplyColor(std::ostream& stream, LogLevel level);

		void Consume(LogLevel level, std::string_view formatted);

		bool ShouldLog(LogLevel level) const noexcept;

	public:
		ConsoleSink();

		void Flush() override;

		void Rotate() override;

		void SetLevel(LogLevel level) noexcept override;

		LogLevel GetLevel() const noexcept override;

		void Log(LogEntity const& entity) override;

	};

	export class FileSink : public ISink {
	private:
		FormatterPtr m_formatter;
		std::ofstream m_file;
		std::filesystem::path m_file_path;
		std::size_t m_max_size;
		std::size_t m_max_files;
		std::size_t m_current_size;
		std::atomic_flag m_stream_mutex;
		std::atomic_uint8_t m_log_level;

	private:
		void CleanOldFiles();

		void Consume(LogLevel level, std::string_view formatted);

		bool ShouldLog(LogLevel level) const noexcept;

	public:
		FileSink(
			std::filesystem::path const& file_path,
			std::size_t max_size = 10u << 20,
			std::size_t max_files = 10u
		);

		FileSink(
			std::filesystem::path const& file_path,
			FormatterPtr const& formatter,
			std::size_t max_size = 10 << 20,
			std::size_t max_files = 10
		);

		~FileSink() noexcept override;

		void SetLevel(LogLevel level) noexcept override;

		LogLevel GetLevel() const noexcept override;

		void Rotate() override;

		void Flush() override;

		void Log(LogEntity const& entity) override;
	};
}