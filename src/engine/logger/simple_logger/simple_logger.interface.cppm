export module simple_logger:interface;
import std;
export import synchronized_function;
export import pointer_wrapper;

namespace fyuu_engine::logger::simple_logger {

	/// @brief 
	export enum class LogLevel : std::uint8_t {
		Trace,
		Debug,
		Info,
		Warning,
		Error,
		Fatal
	};

	export struct LogEntity {
		std::thread::id thread_id = std::this_thread::get_id();
		std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
		std::source_location location;
		std::string message;
		LogLevel level;
		std::string sink;
		bool to_console;
	};

	export class IFormatter {
	public:
		virtual ~IFormatter() = default;
		virtual std::string Format(LogEntity const& log) const = 0;
	};

	export using FormatterPtr = util::PointerWrapper<IFormatter>;

	export class ISink {
	public:
		virtual ~ISink() noexcept = default;

		virtual void SetLevel(LogLevel level) noexcept = 0;
		virtual LogLevel GetLevel() const noexcept = 0;

		virtual void Flush() = 0;
		virtual void Rotate() = 0;
		virtual void Log(LogEntity const& entity) = 0;

	};

	export using SinkPtr = util::PointerWrapper<ISink>;

}