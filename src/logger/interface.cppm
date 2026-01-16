export module logger:interface;
import std;
import collective_resource;

namespace fyuu_engine::logger {

	export using SinkID = std::size_t;
	
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
		std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
		std::source_location location;
		std::string message;
		SinkID sink;
		std::thread::id thread_id = std::this_thread::get_id();
		LogLevel level;
		bool to_console;
	};

	export template <class Derived> class BaseSink {
	private:
		static inline std::atomic<SinkID> s_next_id;

	protected:
		SinkID m_id = s_next_id.fetch_add(1u, std::memory_order::relaxed);

	public:
		SinkID GetID() {
			return m_id;
		}

		void SetLevel(LogLevel level) {
			static_cast<Derived*>(this)->SetLevelImpl(level);
		}

		LogLevel GetLevel() {
			return static_cast<Derived*>(this)->GetLevelImpl();
		}

		void Flush() {
			static_cast<Derived*>(this)->FlushImpl();
		}

		void Rotate() {
			static_cast<Derived*>(this)->RotateImpl();
		}

		void Log(LogEntity const& entity) {
			static_cast<Derived*>(this)->LogImpl(entity);
		}

	};

	export template <class T> concept SinkConcept = requires (T t) {
		{ t.GetID() } -> std::convertible_to<SinkID>;
		{ t.SetLevel(std::declval<LogLevel>()) } -> std::same_as<void>;
		{ t.GetLevel() } -> std::convertible_to<LogLevel>;
		{ t.Flush() } -> std::same_as<void>;
		{ t.Rotate() } -> std::same_as<void>;
		{ t.Log(std::declval<LogEntity>()) } -> std::same_as<void>;
	};

	export template <class Derived> class BaseLogger {
	public:
		void SetLevel(LogLevel level) {
			static_cast<Derived*>(this)->SetLevelImpl(level);
		}

		LogLevel GetLevel() {
			return static_cast<Derived*>(this)->GetLevelImpl();
		}

		void Log(
			LogLevel level,
			std::string_view log_msg,
			std::source_location const& loc = std::source_location::current()
		) {
			static_cast<Derived*>(this)->LogImpl(level, log_msg, loc);
		}

		void Trace(
			std::string_view log_msg,
			std::source_location const& loc = std::source_location::current()
		) {
			Log(LogLevel::Trace, log_msg, loc);
		}

		void Debug(
			std::string_view log_msg,
			std::source_location const& loc = std::source_location::current()
		) {
			Log(LogLevel::Debug, log_msg, loc);
		}

		void Info(
			std::string_view log_msg,
			std::source_location const& loc = std::source_location::current()
		) {
			Log(LogLevel::Info, log_msg, loc);
		}

		void Warning(
			std::string_view log_msg,
			std::source_location const& loc = std::source_location::current()
		) {
			Log(LogLevel::Warning, log_msg, loc);
		}

		void Error(
			std::string_view log_msg,
			std::source_location const& loc = std::source_location::current()
		) {
			Log(LogLevel::Error, log_msg, loc);
		}

		void Fatal(
			std::string_view log_msg,
			std::source_location const& loc = std::source_location::current()
		) {
			Log(LogLevel::Fatal, log_msg, loc);
		}

	};

	export template <class Derived> class BaseCore {
	public:
		class Collector {
		private:
			Derived* m_core;

		public:
			Collector(Derived* core)
				: m_core(core) {

			}

			void operator()(SinkID id) {
				m_core->RemoveSink(id);
			}
		};

		using RegisteredHandle = util::UniqueCollectiveResource<SinkID, Collector>;

		template <SinkConcept Sink>
		RegisteredHandle RegisterSink(Sink&& sink) {
			static_cast<Derived*>(this)->RegisterSinkImpl(sink);
			return RegisteredHandle(sink.GetID(), Collector(static_cast<Derived*>(this)));
		}

	};


}