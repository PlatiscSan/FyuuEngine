module simple_logger:formatter;
namespace logger::simple_logger {

	std::string_view Formatter::LevelToString(core::LogLevel level) noexcept {

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

	std::string Formatter::Format(LogEntity const& log) const {
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

}