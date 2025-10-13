module;
#include <fyuu_logger.h>

export module adapter:clogger;

namespace fyuu_engine::application {

	export class CLoggerAdapter final
		: public ILogger {
	private:
		Fyuu_ILogger* m_logger;

		void LogImp(
			LogLevel level,
			std::source_location const& loc,
			std::string_view message
		) override;

	public:
		CLoggerAdapter(Fyuu_ILogger* logger);

		operator bool() const noexcept;

	};

}