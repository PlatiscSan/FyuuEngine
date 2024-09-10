
#include "pch.h"
#include "Logger.h"

#ifdef ERROR
	#undef ERROR
#endif // ERROR

enum class LogRank {

	NORMAL,
	INFO,
	DEBUG,
	WARNING,
	ERROR,
	FATAL,

};

struct Log {

	std::chrono::system_clock::time_point const timestamp = std::chrono::system_clock::now();
	std::string msg;
	LogRank rank;

};

static std::ofstream s_stream;
static std::queue<Log> s_logs;
static std::mutex s_logs_mutex;


static std::thread s_thread;
static std::condition_variable s_condition;

static std::atomic_bool s_is_quit = true;


static std::unordered_map<LogRank, std::string> s_rank_string = {

	{LogRank::NORMAL, "[Normal] "},

	{LogRank::INFO, "[Info] "},

	{LogRank::DEBUG, "[Debug] "},

	{LogRank::WARNING, "[Warning] "},

	{LogRank::ERROR, "[Error] "},

	{LogRank::FATAL, "[Fatal] "}

};

static void LogThread() {

	while (!s_is_quit) {

		std::unique_lock<std::mutex> logs_lock(s_logs_mutex);
		s_condition.wait(logs_lock, [&]() {return !s_logs.empty() || s_is_quit; });
		if (s_logs.empty() && s_is_quit) {
			return;
		}

		if (!s_logs.empty()) {

			Log log = std::move(s_logs.front());
			s_logs.pop();

			std::time_t raw_time = std::chrono::system_clock::to_time_t(log.timestamp);
			std::tm time_info{};
			localtime_s(&time_info, &raw_time);
			std::array<char, 64> time_string{};
			std::strftime(time_string.data(), time_string.size(), "[%Y-%m-%d %H:%M:%S] ", &time_info);

			s_stream << time_string.data() << s_rank_string[log.rank] << log.msg << std::endl;

#if defined(_DEBUG)
			std::cout << time_string.data() << s_rank_string[log.rank] << log.msg.c_str() << std::endl;
#endif // defined(_DEBUG)

		}

	}

}

void Fyuu::core::log::InitializeLog() {

	if (!s_is_quit) {
		return;
	}

	s_is_quit = false;

	auto now = std::chrono::system_clock::now();
	std::time_t raw_time = std::chrono::system_clock::to_time_t(now);
	std::tm time_info{};

	localtime_s(&time_info, &raw_time);
	std::array<char, 64> file_name{};
	std::strftime(file_name.data(), file_name.size(), "log/%Y-%m-%d.log", &time_info);

	if (!std::filesystem::exists("log")) {
		if (!std::filesystem::create_directories("log")) {
			throw std::runtime_error("Failed to create log directory");
		}
	}

	s_stream.open(file_name.data(), std::ios::app);
	if (!s_stream) {
		throw std::runtime_error("Failed to create log file");
	}

	s_thread = std::thread(LogThread);

}

void Fyuu::core::log::QuitLog() {

	s_is_quit = true;
	s_condition.notify_one();
	s_thread.join();
	s_stream.close();

}



void Fyuu::core::log::Normal(std::string const& msg) {

	std::lock_guard<std::mutex> lock(s_logs_mutex);
	Log log{};
	log.msg = msg;
	log.rank = LogRank::NORMAL;
	s_logs.push(log);
	s_condition.notify_one();

}

void Fyuu::core::log::Info(std::string const& msg) {

	std::lock_guard<std::mutex> lock(s_logs_mutex);
	Log log{};
	log.msg = msg;
	log.rank = LogRank::INFO;
	s_logs.push(log);
	s_condition.notify_one();

}

void Fyuu::core::log::Debug(std::string const& msg) {

	std::lock_guard<std::mutex> lock(s_logs_mutex);
	Log log{};
	log.msg = msg;
	log.rank = LogRank::DEBUG;
	s_logs.push(log);
	s_condition.notify_one();

}

void Fyuu::core::log::Warning(std::string const& msg) {

	std::lock_guard<std::mutex> lock(s_logs_mutex);
	Log log{};
	log.msg = msg;
	log.rank = LogRank::WARNING;
	s_logs.push(log);
	s_condition.notify_one();

}

void Fyuu::core::log::Error(std::string const& msg) {

	std::lock_guard<std::mutex> lock(s_logs_mutex);
	Log log{};
	log.msg = msg;
	log.rank = LogRank::ERROR;
	s_logs.push(log);
	s_condition.notify_one();

}

void Fyuu::core::log::Fatal(std::string const& msg) {

	std::lock_guard<std::mutex> lock(s_logs_mutex);
	Log log{};
	log.msg = msg;
	log.rank = LogRank::FATAL;
	s_logs.push(log);
	s_condition.notify_one();

}
