
#include "pch.h"
#include "ThreadPool.h"
#include "Logger.h"

using namespace Fyuu::core::thread;
using namespace Fyuu::core::log;

static std::queue<Task> s_tasks;
static std::mutex s_task_mutex;
static std::condition_variable s_condition;

static std::queue<std::exception_ptr> s_exceptions;
static std::mutex s_exception_mutex;

static std::list<std::thread> s_pool;
static std::atomic_bool s_is_stop = true;

static void RecordException() {
	
	std::exception_ptr ex = std::current_exception();
	std::lock_guard<std::mutex> lock(s_exception_mutex);
	s_exceptions.push(ex);

}

static void WorkThread() {

	while (!s_is_stop) {

		std::unique_lock<std::mutex> lock(s_task_mutex);
		s_condition.wait(
			lock,
			[]() {
				return s_is_stop.load() || !s_tasks.empty();
			}
		);

		if (s_tasks.empty()) {
			lock.unlock();
			return;
		}

		Task task(std::move(s_tasks.front()));
		s_tasks.pop();
		lock.unlock();

		try {
			task();
		}
		catch (std::exception const& ex) {
			Error(ex.what());
			RecordException();
		}

	}

}

void Fyuu::core::thread::InitializeThreads(std::size_t num_threads) {

	if (!s_is_stop) {
		return;
	}

	std::size_t max_threads = num_threads <= 1 ? 2 : num_threads;
	for (std::size_t i = 0; i < max_threads; ++i) {
		s_pool.emplace_back(WorkThread);
	}

}

void Fyuu::core::thread::StopThreads() {

	s_is_stop = true;
	s_condition.notify_all();
	for (auto& td : s_pool) {
		if (td.joinable()) {
			td.join();
		}
	}

}

void Fyuu::core::thread::CommitTask(Task const& task) {

	std::lock_guard<std::mutex> lock(s_task_mutex);
	s_tasks.push(task);
	s_condition.notify_one();

}

void Fyuu::core::thread::CommitTask(Task&& task) {

	std::lock_guard<std::mutex> lock(s_task_mutex);
	s_tasks.emplace(std::move(task));
	s_condition.notify_one();

}

void Fyuu::core::thread::CheckException() {

	std::unique_lock<std::mutex> lock(s_exception_mutex);
	if (s_exceptions.empty()) {
		return;
	}
	std::exception_ptr ex(std::move(s_exceptions.front()));
	s_exceptions.pop();
	lock.unlock();
	std::rethrow_exception(ex);

}

bool Fyuu::core::thread::IsThreadsRunning() {
	return !s_is_stop;
}
