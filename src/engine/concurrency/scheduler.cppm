export module scheduler;
export import worker;
export import pointer_wrapper;
export import coroutine;
import concurrent_hash_map;
import circular_buffer;
import defer;
import std;

namespace fyuu_engine::concurrency {
	
	export class Scheduler {
	private:
		struct TaskGroup {
			std::size_t total;
			std::coroutine_handle<> coroutine;
			std::atomic_size_t completed;
			TaskGroup(std::size_t total, std::coroutine_handle<> coroutine) noexcept;
			TaskGroup(TaskGroup&& other) noexcept;
			TaskGroup& operator=(TaskGroup&& other) noexcept;
		};
		using Workers = CircularBuffer<util::PointerWrapper<Worker>, 128u>;
		using TaskGroupMap = ConcurrentHashMap<std::size_t, TaskGroup>;

		std::atomic<Workers*> m_workers;
		std::atomic<TaskGroupMap*> m_task_groups;
		std::atomic<std::binary_semaphore*> m_resume_handle;
		std::atomic<std::atomic_size_t*> m_next_id;
		std::jthread m_scheduler_thread;

		static inline ConcurrentHashMap<std::thread::id, Scheduler*> s_thread_instance_map;

		static void Schedule(std::stop_token token);

		void Notify() noexcept;

		Workers* LoadWorkers() const noexcept;

		TaskGroupMap* LoadTaskGroupMap() const noexcept;

		std::size_t AcquireNextID() const noexcept;

	public:
		Scheduler();
		Scheduler(Scheduler&& other) noexcept;
		~Scheduler() noexcept;

		Scheduler& operator=(Scheduler&& other) noexcept;

		template <class... Tasks>
		class TaskAwaitable {
		private:
			Scheduler* m_scheduler;
			std::tuple<Tasks...> m_tasks;
			std::function<void()> Release;
			std::tuple<std::future<std::invoke_result_t<Tasks>>...> m_futures;
			std::exception_ptr m_exception;

		public:
			TaskAwaitable(Scheduler* scheduler, std::tuple<Tasks...>&& tasks)
				: m_scheduler(scheduler), m_tasks(std::move(tasks)) {

			}

			static constexpr bool await_ready() noexcept {
				return false;
			}

			template <class Promise>
			bool await_suspend(std::coroutine_handle<Promise> handle) {

				try {
					if constexpr (IsTask<std::coroutine_handle<Promise>>::value) {
						handle.promise().AddReference();
						Release = [handle]() {
							handle.promise().Release();
							};
					}

					Workers* workers = m_scheduler->LoadWorkers();
					if (workers->empty()) {
						return true;
					}

					TaskGroupMap* task_groups = m_scheduler->LoadTaskGroupMap();
					auto [locked_ptr, success] = task_groups->try_emplace(m_scheduler->AcquireNextID(), sizeof...(Tasks), handle);
					if (!success) {
						throw std::runtime_error("task group creation failed");
					}
					
					/// for capture in lambda without lock
					TaskGroup* task_group = locked_ptr;

					auto submit = [this, workers, task_group](auto&& task) {
						static thread_local std::mt19937 generator(
							static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count())
						);

						std::size_t worker_count = workers->size();
						std::uniform_int_distribution<std::size_t> dist(0, worker_count - 1);
						std::size_t random_index = dist(generator);

						using TaskResult = std::invoke_result_t<decltype(task)>;

						return (*workers)[random_index].Get()->SubmitTask(
							[task, task_group]() {
								util::Defer defer(
									[task_group]() {
										task_group->completed.fetch_add(1u, std::memory_order::release);
									}
								);
								try {
									if constexpr (!std::is_void_v<TaskResult>) {
										return task();
									}
									else {
										task();
									}
								}
								catch (...) {
									throw;
								}
							}
						);
						};

					m_futures = std::apply(
						[submit](auto&&... tasks) {
							return std::make_tuple(submit(tasks)...);
						},
						m_tasks
					);

					m_scheduler->Notify();
					return true;
				}
				catch (std::exception const& ex) {
					std::cerr << ex.what() << std::endl;
					m_exception = std::current_exception();
					return false;
				}

			}

			std::tuple<std::future<std::invoke_result_t<Tasks>>...> await_resume() {

				util::Defer gc(
					[this]() {
						if (Release) {
							Release();
						}
					}
				);

				if (m_exception) {
					std::rethrow_exception(m_exception);
				}
				
				return std::move(m_futures);

			}
		};

		template <class... Conditions>
		class ConditionAwaitable {
			static_assert((std::is_same_v<std::invoke_result_t<Conditions>, bool> && ...),
				"Return value of each condition must be boolean");

		private:
			Scheduler* m_scheduler;
			std::tuple<Conditions...> m_conditions;
			std::function<void()> Release;
			std::exception_ptr m_exception;

			template <class Condition>
			void Submit(TaskGroup* task_group, Condition&& condition) {

				Workers* workers = m_scheduler->LoadWorkers();

				static thread_local std::mt19937 generator(
					static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count())
				);

				std::size_t worker_count = workers->size();
				std::uniform_int_distribution<std::size_t> dist(0, worker_count - 1);
				std::size_t random_index = dist(generator);

				try {
					if (condition()) {
						task_group->completed.fetch_add(
							1u,
							std::memory_order::release
						);
					}
					else {
						m_scheduler->SubmitTask(
							[this, task_group, condition]() {
								Submit(task_group, condition);
							}
						);
					}
				}
				catch (...) {
					task_group->completed.fetch_add(
						1u,
						std::memory_order::release
					);
					throw;
				}
			}

		public:
			ConditionAwaitable(Scheduler* scheduler, std::tuple<Conditions...>&& conditions)
				: m_scheduler(scheduler), m_conditions(std::move(conditions)) {

			}

			static constexpr bool await_ready() noexcept {
				return false;
			}

			template <class Promise>
			bool await_suspend(std::coroutine_handle<Promise> handle) {

				try {
					if constexpr (IsTask<std::coroutine_handle<Promise>>::value) {
						handle.promise().AddReference();
						Release = [handle]() {
							handle.promise().Release();
							};
					}

					Workers* workers = m_scheduler->LoadWorkers();
					if (workers->empty()) {
						return true;
					}

					TaskGroupMap* task_groups = m_scheduler->LoadTaskGroupMap();
					auto [locked_ptr, success] = task_groups->try_emplace(m_scheduler->AcquireNextID(), sizeof...(Conditions), handle);
					if (!success) {
						throw std::runtime_error("task group creation failed");
					}

					/// for capture in lambda without lock
					TaskGroup* task_group = locked_ptr;

					std::apply(
						[this, task_group](auto&&... conditions) {
							(Submit(task_group, conditions), ...);
						},
						m_conditions
					);

					m_scheduler->Notify();
					return true;
				}
				catch (std::exception const& ex) {
					std::cerr << ex.what() << std::endl;
					m_exception = std::current_exception();
					return false;
				}

			}

			void await_resume() {

				util::Defer gc(
					[this]() {
						if (Release) {
							Release();
						}
					}
				);

				if (m_exception) {
					std::rethrow_exception(m_exception);
				}

			}
		};
		Scheduler& Hire(util::PointerWrapper<Worker> const& worker);

		template <class... Tasks>
		auto WaitForTasks(Tasks&&... tasks) {
			return TaskAwaitable<Tasks...>(this, std::tuple<Tasks...>(std::forward<Tasks>(tasks)...));
		}

		template <class... Conditions>
		auto WaitForConditions(Conditions&&... conditions) {
			static_assert(sizeof...(Conditions) > 0,
				"No condition to wait");
			return ConditionAwaitable<Conditions...>(this, std::tuple<Conditions...>(std::forward<Conditions>(conditions)...));
		}

		template <class Func, class... Args>
		auto SubmitTask(Func&& func, Args&&... args) {

			Workers* workers = LoadWorkers();
			util::PointerWrapper<Worker> worker;
			std::size_t retries = 0;
			do {
				worker = std::move(*workers.TryPopFront());
			} while (!worker && ++retries < 10u);

			if (++retries >= 10u) {
				throw std::runtime_error("No available worker");
			}

			auto future = worker->SubmitTask(std::forward<Func>(func), std::forward<Args>(args)...);
			workers->TryEmplaceBack(std::move(worker));

			return future;

		}

	};

}