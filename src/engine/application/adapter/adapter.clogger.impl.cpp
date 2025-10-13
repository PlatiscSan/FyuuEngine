module adapter:clogger;
import std;

namespace fyuu_engine::application {

	void CLoggerAdapter::LogImp(LogLevel level, std::source_location const& loc, std::string_view message)
	{
	}

	CLoggerAdapter::CLoggerAdapter(Fyuu_ILogger* logger)
		: m_logger(logger) {
	}

	CLoggerAdapter::operator bool() const noexcept {
		return m_logger;
	}

}