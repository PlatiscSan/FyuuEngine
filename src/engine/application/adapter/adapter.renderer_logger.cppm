module;
#include <fyuu_logger.h>

export module adapter:renderer_logger;
export import rendering;

namespace fyuu_engine::application {

	export class RendererLoggerAdapter final
		: public core::IRendererLogger {
	private:
		ILogger* m_logger;

	public:
		void Log(
			core::LogLevel level,
			std::string_view message,
			std::source_location const& loc = std::source_location::current()
		) override;

		explicit RendererLoggerAdapter(ILogger* logger) noexcept;
	};
}