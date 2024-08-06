
#include "pch.h"
#include "Logger.h"

using namespace Fyuu;

static std::mutex s_singleton_mutex;
static std::unique_ptr<Logger> s_instance = nullptr;

Logger& Fyuu::Logger::GetInstance() {

	std::lock_guard<std::mutex> lock(s_singleton_mutex);
	if (!s_instance) {
		try {
			s_instance.reset(new Logger());
		}
		catch (std::exception const&) {
			throw std::runtime_error("Failed to initialize logger system");
		}
	}
	return *s_instance.get();

}

void Fyuu::Logger::DestroyInstance() {

	std::lock_guard<std::mutex> lock(s_singleton_mutex);
	if (s_instance) {
		s_instance.reset();
	}

}

Fyuu::Logger::~Logger() {

	m_quit = true;
	m_condition.notify_one();
	m_log_thread.join();
	m_stream.close();

}

void Fyuu::Logger::Normal(std::string const& msg) {

	std::lock_guard<std::mutex> lock(m_mutex);
	m_logs.emplace_back(msg, LOG_NORMAL);
	m_condition.notify_one();

}

void Fyuu::Logger::Info(std::string const& msg) {

	std::lock_guard<std::mutex> lock(m_mutex);
	m_logs.emplace_back(msg, LOG_INFO);
	m_condition.notify_one();

}

void Fyuu::Logger::Debug(std::string const& msg) {

	std::lock_guard<std::mutex> lock(m_mutex);
	m_logs.emplace_back(msg, LOG_DEBUG);
	m_condition.notify_one();

}

void Fyuu::Logger::Warning(std::string const& msg) {

	std::lock_guard<std::mutex> lock(m_mutex);
	m_logs.emplace_back(msg, LOG_WARNING);
	m_condition.notify_one();

}

void Fyuu::Logger::Error(std::string const& msg) {

	std::lock_guard<std::mutex> lock(m_mutex);
	m_logs.emplace_back(msg, LOG_ERROR);
	m_condition.notify_one();

}

void Fyuu::Logger::Fatal(std::string const& msg) {

	std::lock_guard<std::mutex> lock(m_mutex);
	m_logs.emplace_back(msg, LOG_ERROR);
	m_condition.notify_one();

}

Logger::Logger() {

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

	m_stream.open(file_name.data(), std::ios::app);
	if (!m_stream) {
		throw std::runtime_error("Failed to create log file");
	}
	
	m_log_thread = std::move(
		std::thread([&]() {
			while (!m_quit) {
				std::unique_lock<std::mutex> lock(m_mutex);
				m_condition.wait(lock, [&]() {return !m_logs.empty() || m_quit; });
				if (m_quit) {
					return;
				}
				auto logs(std::move(m_logs));
				lock.unlock();
				for (auto& log : logs) {
					WriteLog(log);
				}
				m_logs.clear();
			}
			}
		)
	);

}

void Fyuu::Logger::WriteLog(Log const& log) {

	std::time_t raw_time = std::chrono::system_clock::to_time_t(log.timestamp);
	std::tm time_info{};
	localtime_s(&time_info, &raw_time);
	std::array<char, 64> time_string{};
	std::strftime(time_string.data(), time_string.size(), "[%Y-%m-%d %H:%M:%S] ", &time_info);

	std::string rank_string;
	switch (log.rank) {

	case LOG_NORMAL:
		rank_string = "[Normal] ";
		break;

	case LOG_INFO:
		rank_string = "[Info] ";
		break;

	case LOG_DEBUG:
		rank_string = "[Debug] ";
		break;

	case LOG_WARNING:
		rank_string = "[Warning] ";
		break;

	case LOG_ERROR:
		rank_string = "[Error] ";
		break;

	case LOG_FATAL:
		rank_string = "[Fatal] ";
		break;

	default:
		throw std::runtime_error("Unknown rank");
		break;

	}

	m_stream << time_string.data() << rank_string << log.msg << std::endl;

#if defined(_DEBUG)
	std::printf("%s%s%s\n", time_string.data(), rank_string.c_str(), log.msg.c_str());
#endif // defined(_DEBUG)


}
