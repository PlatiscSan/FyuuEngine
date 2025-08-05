export module simple_logger:formatter;
import :interface;

namespace logger::simple_logger {
	export class Formatter : public IFormatter {
	private:
		static std::string_view LevelToString(core::LogLevel level) noexcept;

	public:
		std::string Format(LogEntity const& log) const override;

	};
}