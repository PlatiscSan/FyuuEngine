module;
#if defined(_WIN32)
#include <Windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#pragma comment(lib, "pdh.lib")

#elif defined(__linux__)
#include <unistd.h>

#elif defined(__APPLE__)
#include <mach/mach_init.h>
#include <mach/mach_error.h>
#include <mach/mach_host.h>
#include <mach/vm_map.h>
#endif

#ifdef ERROR
	#undef ERROR
#endif // ERROR

export module thread_pool;
export import logger_interface;

import std;
import circular_buffer;
import concurrent_hash_set;
import concurrent_vector;
import defer;
import synchronized_function;

export namespace concurrency {

	class Worker {
	public:
		using Task = std::function<void()>;

		/// @brief use concurrent circular buffer as queue.
		using TaskQueue = CircularBuffer<Task, 32u>;

		using EmptyTaskQueueNotifier = SynchronizedFunction<bool(Worker*)>;

	private:
		std::optional<std::thread::id> m_id;
		std::optional<std::stop_source> m_stop_source;
		std::atomic<TaskQueue*> m_tasks;
		std::atomic<EmptyTaskQueueNotifier*> m_notifier;
		std::atomic<std::binary_semaphore*> m_semaphore;

		/// @brief	try to fetch work from queue then proceed it
		/// @return true if any task is proceeded
		static bool FetchTask(TaskQueue& tasks) {

			std::size_t proceeded_count = 0;
			bool empty = false;
			do {
				empty = tasks.empty();
				if (!empty) {
					std::optional<Task> task = tasks.TryPopFront();
					if (task) {
						try {
							(*task)();
							++proceeded_count;
						}
						catch (...) {
							
						}
					}
				}
			} while (!empty);

			return proceeded_count != 0;

		}

		static void Work(Worker* worker, std::stop_token const& token) {

			std::binary_semaphore semaphore(0);
			std::stop_callback stop_callback(
				token,
				[&semaphore]() {
					semaphore.release();
				}
			);

			TaskQueue tasks;
			EmptyTaskQueueNotifier notifier;

			worker->m_tasks.store(&tasks, std::memory_order::release);
			worker->m_notifier.store(&notifier, std::memory_order::release);

			do {

				if (!Worker::FetchTask(tasks)) {
					// No task proceeded

					{
						auto safe_is_callable = notifier.IsCallable();
						if (safe_is_callable) {
							// notify task done;
							if (notifier.UnsafeCall(worker)) {
								continue;
							}
						}
					}

					// set handle to resume;
					worker->m_semaphore.store(&semaphore, std::memory_order::release);

					// wait for resume
					semaphore.acquire();

					// clear handle
					worker->m_semaphore.store(nullptr, std::memory_order::release);
				}

			} while (!token.stop_requested());

			Worker::FetchTask(tasks);

		}

	public:
		SynchronizedFunction<void()> const Run;
		SynchronizedFunction<void()> const Stop;

		Worker() :
			Run(
				[this]() {
					using namespace std::chrono_literals;

					if (m_tasks.load(std::memory_order::acquire)) {
						return;
					}

					std::jthread worker_thread(
						[this](std::stop_token const& token) {
							Worker::Work(this, token);
						}
					);

					worker_thread.detach();
					m_id.emplace(worker_thread.get_id());
					m_stop_source.emplace(worker_thread.get_stop_source());

					TaskQueue* tasks;
					do {
						tasks = m_tasks.load(std::memory_order::acquire);
						if (!tasks) {
							std::this_thread::yield();
						}
					} while (!tasks);

				}
			), 
			Stop(
				[this]() {
					if (m_stop_source) {
						m_stop_source->request_stop();
						m_stop_source.reset();
					}
				}
			) {

		}
		Worker(Worker const&) = delete;
		Worker(Worker&&) = delete;

		Worker& operator=(Worker const&) = delete;
		Worker& operator=(Worker&&) = delete;

		std::optional<std::thread::id> GetID() const noexcept {
			return m_id;
		}

		void Resume() noexcept {
			auto semaphore = m_semaphore.exchange(nullptr, std::memory_order::acq_rel);
			if (semaphore) {
				semaphore->release();
			}
		}

		~Worker() noexcept {
			Stop();
		}

		template <std::convertible_to<EmptyTaskQueueNotifier> Compatible>
		void SetEmptyTaskQueueNotifier(Compatible&& compatible_notifier) {
			EmptyTaskQueueNotifier* notifier = nullptr;
			do {
				notifier = m_notifier.load(std::memory_order::acquire);
				if (!notifier) {
					std::this_thread::yield();
				}
			} while (!notifier);
			*notifier = std::forward<Compatible>(compatible_notifier);
		}

		template <class Func, class... Args>
		auto SubmitTask(Func&& func, Args&&... args) {
			using Result = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;

			TaskQueue* tasks;
			do {
				tasks = m_tasks.load(std::memory_order::acquire);
				if (!tasks) {
					Run();
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
					catch (...) {
						promise->set_exception(std::current_exception());
					}
				}
			);

			Resume();

			return promise->get_future();

		}

		template <std::convertible_to<Task> CompatibleTask>
		void SubmitExistingTask(CompatibleTask&& task) {
			if (auto tasks = m_tasks.load(std::memory_order::acquire)) {
				tasks->emplace_back(std::forward<CompatibleTask>(task));
				Resume();
			}
		}

		std::size_t TaskCount() const noexcept {
			auto task = m_tasks.load(std::memory_order::acquire);
			return task ? task->size() : 0u;
		}

		bool HasTask() const noexcept {
			auto task = m_tasks.load(std::memory_order::acquire);
			return task ? !task->empty() : false;
		}

		std::optional<Task> RemoveOneTask() {
			auto task = m_tasks.load(std::memory_order::acquire);
			return task ? task->TryPopFront() : std::nullopt;
		}

		float Workload() const noexcept {
			auto task = m_tasks.load(std::memory_order::acquire);
			return task ? static_cast<float>(task->size()) / task->Capacity : 0.0f;
		}

	};

	class DelayTaskQueue {
	private:
		struct Task {
		private:
			static std::size_t GetID() noexcept {
				static std::atomic_size_t next = 0;
				return next.fetch_add(1u, std::memory_order::relaxed);
			}

		public:
			std::chrono::steady_clock::time_point trigger;
			typename Worker::Task task;
			std::size_t id = GetID();

			Task(Task&&) noexcept = default;
			Task& operator=(Task&&) noexcept = default;

			bool operator<(Task const& other) const noexcept {
				return trigger < other.trigger;
			}

			Task(std::chrono::milliseconds delay, typename Worker::Task const& task)
				: trigger(std::chrono::steady_clock::now() + delay), task(task) {

			}

		};

		std::priority_queue<Task, std::deque<Task>> m_tasks;
		mutable std::mutex m_tasks_mutex;

		ConcurrentHashSet<std::size_t> m_canceled_tasks;

	public:
		template <class Func, class... Args>
		auto PushScheduledTask(std::chrono::milliseconds delay, Func&& func, Args&&... args) {

			using Result = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;

			auto promise = std::make_shared<std::promise<Result>>();

			Task task(
				delay,
				[func = std::forward<Func>(func), ...args = std::forward<Args>(args), promise]() {
					try {
						if constexpr (std::is_void_v<Result>) {
							std::invoke(func, args...);
							promise->set_value();
						}
						else {
							promise->set_value(std::invoke(func, args...));
						}
					}
					catch (...) {
						promise->set_exception(std::current_exception());
					}
				}
			);

			auto id = task.id;

			{
				std::lock_guard<std::mutex> lock(m_tasks_mutex);
				m_tasks.emplace(std::move(task));
			}

			return std::make_pair(id, promise->get_future());

		}

		void CancelTask(std::size_t id) {
			m_canceled_tasks.emplace(id);
		}

		std::optional<std::chrono::steady_clock::time_point> NextReady() {
			do {
				std::lock_guard<std::mutex> lock(m_tasks_mutex);
				if (m_tasks.empty()) {
					return std::nullopt;
				}
				auto& top = m_tasks.top();
				auto locked_pos = m_canceled_tasks.find(top.id);
				if (locked_pos) {
					m_canceled_tasks.erase(top.id);
					m_tasks.pop();
				}
				else {
					return m_tasks.top().trigger;
				}
			} while (true);
		}

		std::optional<typename Worker::Task> Ready() {
			do {
				std::lock_guard<std::mutex> lock(m_tasks_mutex);
				if (m_tasks.empty() || std::chrono::steady_clock::now() < m_tasks.top().trigger) {
					return std::nullopt;
				}
				auto& top = m_tasks.top();
				auto locked_pos = m_canceled_tasks.find(top.id);
				if (locked_pos) {
					m_canceled_tasks.erase(top.id);
					m_tasks.pop();
				}
				else {
					if (std::chrono::steady_clock::now() < top.trigger) {
						typename Worker::Task task = std::move(m_tasks.top().task);
						m_tasks.pop();
						return task;
					}
					else {
						return std::nullopt;
					}
				}
			} while (true);
		}

	};

	class ThreadPool : public util::EnableSharedFromThis<ThreadPool> {
	public:
		using Base = util::EnableSharedFromThis<ThreadPool>;
		using ThreadPoolPtr = typename Base::Pointer;

	private:
		struct MonitorContext {
			util::PointerWrapper<ThreadPool> pool;
			std::chrono::steady_clock::time_point resume_point;
			std::chrono::steady_clock::time_point last_adjustment;
			std::binary_semaphore semaphore{ 0 };
			MonitorContext(util::PointerWrapper<ThreadPool> const& pool)
				: pool(pool), last_adjustment(std::chrono::steady_clock::now()) {

			}
		};

		ConcurrentVector<std::unique_ptr<Worker>> m_workers;
		DelayTaskQueue m_delay_queue;
		std::optional<Worker> m_monitor;
		core::LoggerPtr m_logger = nullptr;
		std::chrono::milliseconds m_adjustment_interval;
		std::chrono::milliseconds m_polling_interval;
		std::atomic<std::binary_semaphore*> m_semaphore;
		std::atomic_size_t m_min;
		std::atomic_size_t m_max;
		std::atomic_flag m_running;

		static inline SynchronizedFunction<std::size_t(std::size_t)> RandomIndex{
			[](std::size_t worker_count) {
				static std::mt19937 rng(std::random_device{}());
				std::uniform_int_distribution<std::size_t> dist(0, worker_count - 1);
				return dist(rng);
			}
		};


		static double GetSystemCPULoad() {
			static auto last_time = std::chrono::steady_clock::now();
			static auto last_idle = 0ULL;
			static auto last_total = 0ULL;

			const auto now = std::chrono::steady_clock::now();
			const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time);

			// Avoid sampling too frequently
			if (elapsed.count() < 500) {
				static double last_load = 0.0;
				return last_load;
			}

			last_time = now;

#if defined(_WIN32)
			// Windows implementation (using PDH API)
			static PDH_HQUERY cpu_query;
			static PDH_HCOUNTER cpu_total;

			if (!cpu_query) {
				PdhOpenQuery(nullptr, 0, &cpu_query);
				PdhAddEnglishCounter(cpu_query, TEXT("\\Processor(_Total)\\% Processor Time"), 0, &cpu_total);
				PdhCollectQueryData(cpu_query);
				return 0.0;
			}

			PDH_STATUS status = PdhCollectQueryData(cpu_query);
			if (status != ERROR_SUCCESS) {
				return 0.0;
			}

			PDH_FMT_COUNTERVALUE counter_val;
			status = PdhGetFormattedCounterValue(cpu_total, PDH_FMT_DOUBLE, nullptr, &counter_val);
			if (status != ERROR_SUCCESS) {
				return 0.0;
			}

			double load = counter_val.doubleValue / 100.0;
			return (load > 1.0) ? 1.0 : (load < 0.0 ? 0.0 : load);

#elif defined(__linux__)
			// Linux implementation (parsing /proc/stat)
			std::ifstream proc_stat("/proc/stat");
			proc_stat.ignore(5, ' '); // Skip the 'cpu' prefix

			std::vector<unsigned long long> cpu_times;
			unsigned long long time;
			while (proc_stat >> time) {
				cpu_times.push_back(time);
			}

			if (cpu_times.size() < 4) {
				return 0.0;
			}

			// Calculate total CPU time
			unsigned long long total_time = 0;
			for (auto t : cpu_times) {
				total_time += t;
			}

			// Calculate idle time (idle + iowait)
			unsigned long long idle_time = cpu_times[3] + (cpu_times.size() > 4 ? cpu_times[4] : 0);

			// First call to initialize
			if (last_total == 0) {
				last_total = total_time;
				last_idle = idle_time;
				return 0.0;
			}

			// Calculate the difference
			unsigned long long total_delta = total_time - last_total;
			unsigned long long idle_delta = idle_time - last_idle;

			// Update the last value
			last_total = total_time;
			last_idle = idle_time;

			// Avoid division by zero error
			if (total_delta == 0) {
				return 0.0;
			}

			double load = 1.0 - (static_cast<double>(idle_delta) / total_delta;
			return (load > 1.0) ? 1.0 : (load < 0.0 ? 0.0 : load);

#elif defined(__APPLE__)
			// macOS implementation (using Mach API)
			host_cpu_load_info_data_t cpu_info;
			mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;

			if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
				reinterpret_cast<host_info_t>(&cpu_info),
				&count) != KERN_SUCCESS) {
				return 0.0;
			}

			// Calculate total CPU time
			unsigned long long total_time =
				cpu_info.cpu_ticks[CPU_STATE_USER] +
				cpu_info.cpu_ticks[CPU_STATE_SYSTEM] +
				cpu_info.cpu_ticks[CPU_STATE_NICE] +
				cpu_info.cpu_ticks[CPU_STATE_IDLE];

			// Calculate idle time
			unsigned long long idle_time = cpu_info.cpu_ticks[CPU_STATE_IDLE];

			// First call to initialize
			if (last_total == 0) {
				last_total = total_time;
				last_idle = idle_time;
				return 0.0;
			}

			// Calculate the difference
			unsigned long long total_delta = total_time - last_total;
			unsigned long long idle_delta = idle_time - last_idle;

			// Update the last value
			last_total = total_time;
			last_idle = idle_time;

			// Avoid division by zero error
			if (total_delta == 0) {
				return 0.0;
			}

			double load = 1.0 - (static_cast<double>(idle_delta) / total_delta);
			return (load > 1.0) ? 1.0 : (load < 0.0 ? 0.0 : load);

#else
			// Unsupported operating system
			return 0.0;
#endif
		}

		static void HandleSuspend(std::shared_ptr<MonitorContext> const& ctx) {

			ThreadPool* pool = ctx->pool;

			if (!pool->m_running.test(std::memory_order::acquire)) {
				return;
			}


			auto& semaphore = pool->m_semaphore;
			semaphore.store(&ctx->semaphore, std::memory_order::release);
			(void)ctx->semaphore.try_acquire_for(pool->m_polling_interval);
			semaphore.store(nullptr, std::memory_order::release);

			pool->m_monitor->SubmitTask(
				[ctx]() mutable {
					ThreadPool::HandleScheduledTask(ctx);
				}
			);

		}

		static void HandleAdjustment(std::shared_ptr<MonitorContext> const& ctx) {

			ThreadPool* pool = ctx->pool;

			if (!pool->m_running.test(std::memory_order::acquire)) {
				return;
			}

			try {

				double system_load = GetSystemCPULoad();

				if (pool->m_logger) {

					auto percentage = system_load * 100.0;

					pool->m_logger->Log(
						core::LogLevel::DEBUG,
						std::source_location::current(),
						"System CPU load: {:.2f}%",
						percentage
					);
				}

				double total_workload = 0.0;
				std::size_t worker_count = pool->m_workers.size();

				if (worker_count > 0) {
					auto locked_workers = pool->m_workers.LockedView();
					for (auto& worker_ptr : locked_workers) {
						total_workload += worker_ptr->Workload();
					}
					double avg_workload = total_workload / worker_count;

					if (pool->m_logger) {
						auto percentage = avg_workload * 100.0;
						pool->m_logger->Log(
							core::LogLevel::DEBUG,
							std::source_location::current(),
							"Average worker workload: {:.2f}%",
							percentage
						);
					}

					std::size_t min = pool->m_min.load(std::memory_order::relaxed);
					std::size_t max = pool->m_max.load(std::memory_order::relaxed);
					std::size_t current_count = worker_count;

					if (system_load < 0.7 && avg_workload > 0.7 && current_count < max) {
						std::size_t wanted = static_cast<std::size_t>(current_count * 1.5);
						wanted = (std::min)(wanted, max);

						if (pool->m_logger) {
							pool->m_logger->Log(
								core::LogLevel::INFO,
								std::source_location::current(),
								"Scaling up workers: {} -> {}",
								current_count, 
								wanted
							);
						}

						pool->HireWorker(wanted - current_count);
					}
					else if ((system_load > 0.8 || avg_workload < 0.3) && current_count > min) {
						if (pool->m_logger) {
							auto new_count = current_count - 1;
							pool->m_logger->Log(
								core::LogLevel::INFO,
								std::source_location::current(),
								"Scaling down workers: {} -> {}",
								current_count, 
								new_count
							);
						}

						if (auto worker = pool->m_workers.pop_back()) {
							(*worker)->Stop();

							if (pool->m_logger) {
								if (auto worker_id = (*worker)->GetID()) {
									pool->m_logger->Debug(
										std::source_location::current(),
										"Worker {} released",
										*worker_id
									);
								}
							}

							static thread_local std::mt19937 rng(std::random_device{}());
							std::uniform_int_distribution<std::size_t> dist(0, pool->m_workers.size() - 1);
							std::size_t redistributed = 0;
							do {
								if (auto task = (*worker)->RemoveOneTask()) {
									auto locked_ref = pool->m_workers[dist(rng)];
									std::unique_ptr<Worker>& ref = locked_ref;
									ref->SubmitExistingTask(std::move(*task));
									++redistributed;
								}
							} while ((*worker)->HasTask());

							if (pool->m_logger) {
								pool->m_logger->Log(
									core::LogLevel::DEBUG,
									std::source_location::current(),
									"Redistributed {} tasks from released worker",
									redistributed
								);
							}
						}
					}
				}
			}
			catch (std::exception const& ex) {
				std::string_view ex_message = ex.what();
				if (pool->m_logger) {
					pool->m_logger->Log(
						core::LogLevel::ERROR,
						std::source_location::current(),
						"Exception in adjustment: {}",
						ex_message
					);
				}
			}
			catch (...) {
				if (pool->m_logger) {
					pool->m_logger->Log(
						core::LogLevel::ERROR,
						std::source_location::current(),
						"Unknown exception in adjustment"
					);
				}
			}

			// Arrange the next adjustment
			std::chrono::steady_clock::time_point next_adjustment
				= std::chrono::steady_clock::now() + pool->m_adjustment_interval;

			ctx->resume_point = (std::min)(ctx->resume_point, next_adjustment);

			pool->m_monitor->SubmitTask(
				[ctx]() mutable {
					ThreadPool::HandleSuspend(ctx);
				}
			);

		}

		static void HandleScheduledTask(std::shared_ptr<MonitorContext> const& ctx) {

			ThreadPool* pool = ctx->pool;

			if (!pool->m_running.test(std::memory_order::acquire)) {
				return;
			}

			auto& delay_queue = pool->m_delay_queue;
			auto& workers = pool->m_workers;

			if (auto task = pool->m_delay_queue.Ready()) {

				if (pool->m_logger) {
					pool->m_logger->Log(
						core::LogLevel::TRACE,
						std::source_location::current(),
						"Processing delayed task"
					);
				}

				auto locked_ref = workers[ThreadPool::RandomIndex(pool->m_workers.size())];
				std::unique_ptr<Worker>& ref = locked_ref;
				ref->SubmitExistingTask(std::move(*task));

			}
			else {
				if (auto next_ready = delay_queue.NextReady()) {
					ctx->resume_point = (std::min)(ctx->resume_point, *next_ready);
				}
			}

			pool->m_monitor->SubmitTask(
				[ctx]() mutable {
					ThreadPool::HandleAdjustment(ctx);
				}
			);

		}

		bool StealWork(Worker* free_worker) {

			if (m_workers.empty()) {

				// the worker now can take a rest.

				return false;
			}

			bool stolen = false;
			for (std::size_t i = 0; i < 10; i++) {
				auto locked_ref = m_workers[ThreadPool::RandomIndex(m_workers.size())];
				std::unique_ptr<Worker>& ref = locked_ref;
				if (ref.get() == free_worker) {
					continue;
				}
				if (auto task = ref->RemoveOneTask()) {
					stolen = true;
					free_worker->SubmitExistingTask(std::move(*task));
					break;
				}
			}

			return stolen;

		}

		void HireWorker(std::size_t wanted) {

			std::size_t current_size = m_workers.size();
			std::size_t max = m_max.load(std::memory_order::acquire);

			if (current_size >= max) {
				return;
			}

			std::size_t actual_count = (std::min)(wanted, max - current_size);
			if (actual_count == 0) {
				return;
			}

			for (std::size_t i = 0; i < actual_count; ++i) {
				auto worker = std::make_unique<Worker>();
				worker->Run();
				worker->SetEmptyTaskQueueNotifier(
					[pool = This()](Worker* free_worker) {
						return pool->StealWork(free_worker);
					}
				);
				auto safe_ref = m_workers.emplace_back(std::move(worker));
			}

		}

		void EmployManager() {

			m_running.test_and_set(std::memory_order::release);

			auto ctx = std::make_shared<MonitorContext>(This());

			m_monitor.emplace().SubmitTask(
				[ctx]() mutable {
					ThreadPool::HandleScheduledTask(ctx);
				}
			);

		}

		void ResumeMonitor() {
			if (!m_running.test(std::memory_order::acquire)) {
				return;
			}
			auto semaphore = m_semaphore.exchange(nullptr, std::memory_order::acq_rel);
			if (semaphore) {
				semaphore->release();
			}
		}

	public:
		ThreadPool(
			std::size_t min, 
			std::size_t max, 
			std::chrono::milliseconds adjustment_interval = std::chrono::milliseconds(5000),
			std::chrono::milliseconds polling_interval = std::chrono::milliseconds(1000)
		) : m_min(min), 
			m_max(max),
			m_adjustment_interval(adjustment_interval), 
			m_polling_interval(polling_interval) {

		}

		template <std::convertible_to<core::LoggerPtr> Logger>
		ThreadPool(
			std::size_t min,
			std::size_t max,
			Logger&& core,
			std::chrono::milliseconds adjustment_interval = std::chrono::milliseconds(5000),
			std::chrono::milliseconds polling_interval = std::chrono::milliseconds(1000)
		) : m_min(min),
			m_max(max),
			m_logger(util::MakeReferred(std::forward<Logger>(core))),
			m_adjustment_interval(adjustment_interval),
			m_polling_interval(polling_interval) {

			if (m_logger) {

				m_logger->Log(
					core::LogLevel::INFO,
					std::source_location::current(),
					"ThreadPool created: min={}, max={}, adjustment={}, polling={}",
					min,
					max,
					m_adjustment_interval,
					m_polling_interval
				);
			}

		}

		void Stop() {
			if (!m_running.test(std::memory_order::acquire)) {
				return;
			}

			{
				auto workers = m_workers.LockedModifier();
				for (auto& worker : workers) {
					worker->Stop();
				}
			}

			m_running.clear(std::memory_order::release);
			ResumeMonitor();

			m_workers.clear();
			m_monitor.reset();

		}

		~ThreadPool() noexcept {
			Stop();
		}

		void Run() {

			if (!m_running.test_and_set(std::memory_order::acquire)) {
				EmployManager();
				HireWorker(m_min.load(std::memory_order::relaxed));
			}

		}

		template <class Func, class... Args>
		auto SubmitTask(Func&& func, Args&&... args) {

			Run();

			auto locked_ref = m_workers[ThreadPool::RandomIndex(ThreadPool::m_workers.size())];
			std::unique_ptr<Worker>& ref = locked_ref;

			if (m_logger) {
				m_logger->Trace(
					std::source_location::current(),
					"Submitting task"
				);
			}

			return ref->SubmitTask(std::forward<Func>(func), std::forward<Args>(args)...);

		}

		template <class Func, class... Args>
		auto SubmitScheduledTask(std::chrono::milliseconds delay, Func&& func, Args&&... args) {

			Run();

			if (m_logger) {
				auto delay_count = delay.count();
				m_logger->Log(
					core::LogLevel::TRACE,
					std::source_location::current(),
					"Submitting scheduled task (delay={}ms)",
					delay_count
				);
			}

			util::Defer defer = [this]() {
				ResumeMonitor();
				};
			return m_delay_queue.PushScheduledTask(delay, std::forward<Func>(func), std::forward<Args>(args)...);

		}

		void CancelScheduledTask(std::size_t id) {
			m_delay_queue.CancelTask(id);
		}

		std::vector<float> Workloads() {
			std::vector<float> workloads;
			auto workers = m_workers.LockedView();
			for (auto& worker : workers) {
				workloads.emplace_back(worker->Workload());
			}
			return workloads;
		}

		std::size_t WorkerCount() {
			return m_workers.size();
		}

		core::LoggerPtr GetLogger() const noexcept {
			return m_logger;
		}

	};

	using ThreadPoolPtr = typename ThreadPool::ThreadPoolPtr;

}