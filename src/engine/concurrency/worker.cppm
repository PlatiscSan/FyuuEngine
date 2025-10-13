export module worker;
import circular_buffer;
import concurrent_hash_map;
import std;

namespace fyuu_engine::concurrency {
	export class Worker {
	private:
		using Task = std::function<void()>;
		using TaskQueue = CircularBuffer<Task, 64u>;

		std::jthread m_thread;
		std::atomic<TaskQueue*> m_tasks;
		std::atomic<std::binary_semaphore*> m_resume_handle;

		static inline concurrency::ConcurrentHashMap<std::thread::id, Worker*> s_thread_instance_map;

		void Work(std::stop_token stop_token);

		void Notify() noexcept;

	public:
		Worker();
		Worker(Worker&& other) noexcept;
		~Worker() noexcept;

		Worker& operator=(Worker&& other) noexcept;

		template <class Func, class... Args>
		auto SubmitTask(Func&& func, Args&&... args) {
			using Result = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;

			if (!m_thread.joinable()) {
				throw std::runtime_error("invalid worker");
			}

			TaskQueue* tasks;
			do {
				tasks = m_tasks.load(std::memory_order::acquire);
				if (!tasks) {
					std::this_thread::yield();
				}
			} while (!tasks);

			auto promise = std::make_shared<std::promise<Result>>();

			tasks->emplace_back(
				[func = std::forward<Func>(func), ...args = std::forward<Args>(args), promise]() mutable {
					try {
						if constexpr (std::is_void_v<Result>) {
							std::invoke(func, args...);
							promise->set_value();
						}
						else {
							promise->set_value(std::invoke(func, args...));
						}
					}
					catch (std::exception const& ex) {
						std::cerr 
							<< "[Warning] Unhandled exception thrown, error message: "
							<< ex.what() 
							<< "\n";
						promise->set_exception(std::current_exception());
					}
				}
			);

			Notify();

			return promise->get_future();

		}

	};
}