module adapter:simple_logger;

namespace fyuu_engine::application {

	void SimpleLoggerAdapter::LogImp(LogLevel level, std::source_location const& loc, std::string_view message) {
		m_logger->Log(static_cast<logger::simple_logger::LogLevel>(level), loc, "{}", message);
	}

	SimpleLoggerAdapter::SimpleLoggerAdapter(logger::simple_logger::Logger* logger)
		: m_logger(logger) {
	}

}