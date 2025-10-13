module adapter:renderer_logger;

namespace fyuu_engine::application {

	void RendererLoggerAdapter::Log(
		core::LogLevel level,
		std::string_view message,
		std::source_location const& loc
	) {
		m_logger->Log(static_cast<LogLevel>(level), loc, "{}", message);
	}

	RendererLoggerAdapter::RendererLoggerAdapter(ILogger* logger) noexcept
		: m_logger(logger) {
	}

}