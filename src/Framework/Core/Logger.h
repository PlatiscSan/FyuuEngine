#ifndef LOGGER_H
#define LOGGER_H

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <shared_mutex>
#include <thread>

#if defined(GetMessage)
	#undef GetMessage
#endif // defined(GetMessage)

namespace Fyuu {

	class Logger final {

	public:

		Logger(Logger const&) = delete;
		Logger& operator=(Logger const&) = delete;
		Logger(Logger&&) = delete;
		Logger& operator=(Logger&&) = delete;

		static Logger& GetInstance();
		static void DestroyInstance();

		~Logger() noexcept;

		void Normal(std::string const& msg);
		void Info(std::string const& msg);
		void Debug(std::string const& msg);
		void Warning(std::string const& msg);
		void Error(std::string const& msg);
		void Fatal(std::string const& msg);


	private:

		enum LogRank {

			LOG_NORMAL,
			LOG_INFO,
			LOG_DEBUG,
			LOG_WARNING,
			LOG_ERROR,
			LOG_FATAL,

		};

		struct Log {

			std::chrono::system_clock::time_point const timestamp = std::chrono::system_clock::now();
			std::string msg;
			LogRank rank;

			Log(std::string const& _msg, LogRank _rank)
				:msg(_msg), rank(_rank) {}

		};

	private:

		Logger();

		void WriteLog(Log const& log);

	private:

		std::ofstream m_stream;
		std::condition_variable m_condition;
		std::mutex m_mutex;
		std::list<Log> m_logs;
		std::thread m_log_thread;
		std::atomic_bool m_quit = false;

	};

}

#endif // !LOGGER_H
