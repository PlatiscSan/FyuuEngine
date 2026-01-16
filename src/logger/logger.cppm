export module logger;
import :simple_logger_sink;
import :simple_logger_core;

namespace fyuu_engine::logger {

	export using FileSink = simple_logger::FileSink<simple_logger::Formatter>;
	export using ConsoleSink = simple_logger::ConsoleSink<simple_logger::Formatter>;
	export using Core = simple_logger::LoggingCore<FileSink, ConsoleSink>;
	export using Logger = typename Core::Logger;

}