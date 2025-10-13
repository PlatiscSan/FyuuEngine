module;
#include <fyuu_logger.h>

export module adapter:simple_logger;
export import simple_logger;

namespace fyuu_engine::application {

	export class SimpleLoggerAdapter final 
		: public ILogger {
	private:
		logger::simple_logger::Logger* m_logger;

		void LogImp(
			LogLevel level,
			std::source_location const& loc,
			std::string_view message
		) override;

	public:
		SimpleLoggerAdapter(logger::simple_logger::Logger* logger);
	};

}