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

namespace Fyuu::core::log {

	void InitializeLog();
	void QuitLog();

	void Normal(std::string const& msg);
	void Info(std::string const& msg);
	void Debug(std::string const& msg);
	void Warning(std::string const& msg);
	void Error(std::string const& msg);
	void Fatal(std::string const& msg);


}

#endif // !LOGGER_H
