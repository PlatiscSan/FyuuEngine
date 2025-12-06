module logger:simple_logger_sink;
import defer;
import std;

namespace fyuu_engine::logger::simple_logger {

	std::string_view Formatter::LevelToString(LogLevel level) noexcept {
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

	std::string Formatter::operator()(LogEntity const& log) {
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