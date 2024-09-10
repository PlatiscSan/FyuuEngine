#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <future>
#include <thread>

namespace Fyuu::core::thread {

	using Task = std::function<void()>;

	void InitializeThreads(std::size_t num_threads = std::thread::hardware_concurrency());
	void StopThreads();
	void CommitTask(Task const& task);
	void CommitTask(Task&& task);
	void CheckException();
	bool IsThreadsRunning();

	template <class Func, class... Arg>
	auto CommitAsyncTask(Func&& func, Arg&&... args) 
		-> std::future<decltype(std::forward<Func>(func)(std::forward<Arg>(args)...))> {

		using RetType = decltype(std::forward<Func>(func)(std::forward<Arg>(args)...));

		if (!IsThreadsRunning()) {
			return std::future<RetType>();
		}

		auto task = std::make_shared<std::packaged_task<RetType()>>(
			std::bind(std::forward<Func>(func), std::forward<Arg>(args)...)
		);
		std::future<RetType> ret = task->get_future();

		CommitTask([task]() {(*task)(); });

		return ret;

	}


}

#endif // !THREAD_POOL_H
