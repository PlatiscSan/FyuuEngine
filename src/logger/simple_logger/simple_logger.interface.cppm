export module simple_logger:interface;
import std;
export import logger;
export import synchronized_function;
export import pointer_wrapper;

namespace logger::simple_logger {

	export struct LogEntity {
		std::thread::id thread_id = std::this_thread::get_id();
		std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
		std::source_location location;
		std::string message;
		core::LogLevel level;
		std::string sink;
		bool to_console;
	};

	export class IFormatter {
	public:
		virtual ~IFormatter() = default;
		virtual std::string Format(LogEntity const& log) const = 0;
	};

	export using FormatterPtr = util::PointerWrapper<IFormatter>;

	export class BaseSink {
	private:
		FormatterPtr m_formatter;

	protected:
		virtual bool ShouldLog(core::LogLevel level) const noexcept = 0;
		virtual void Consume(core::LogLevel level, std::string_view formatted) = 0;
		virtual void FlushImp() = 0;
		virtual void RotateImp() = 0;

	public:
		concurrency::SynchronizedFunction<void(LogEntity)> const Log;
		concurrency::SynchronizedFunction<void()> const Flush;
		concurrency::SynchronizedFunction<void()> const Rotate;;

		template <std::convertible_to<FormatterPtr> Formatter>
		explicit BaseSink(Formatter&& formatter)
			: m_formatter(std::forward<Formatter>(formatter)),
			Log(
				[this](LogEntity const& log) {
					if (ShouldLog(log.level)) {
						Consume(log.level, m_formatter->Format(log));
					}
				}
			),
			Flush(
				[this]() {
					FlushImp();
				}
			),
			Rotate(
				[this]() {
					RotateImp();
				}
			) {

		}

		virtual ~BaseSink() noexcept = default;

		virtual void SetLevel(core::LogLevel level) noexcept = 0;
		virtual core::LogLevel GetLevel() const noexcept = 0;

	};

	export using SinkPtr = util::PointerWrapper<BaseSink>;

}