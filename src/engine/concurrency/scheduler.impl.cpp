module scheduler;

namespace fyuu_engine::concurrency {

	Scheduler::TaskGroup::TaskGroup(std::size_t total, std::coroutine_handle<> coroutine) noexcept 
		: total(total), coroutine(coroutine), completed(0) {
	}

	Scheduler::TaskGroup::TaskGroup(TaskGroup&& other) noexcept
		: total(std::exchange(other.total, 0)), 
		coroutine(std::move(other.coroutine)),
		completed(other.completed.exchange(0, std::memory_order::relaxed)) {
	}

	Scheduler::TaskGroup& Scheduler::TaskGroup::operator=(TaskGroup&& other) noexcept {
		if (this != &other) {
			total = std::exchange(other.total, 0);
			coroutine = std::move(other.coroutine);
			completed.store(
				other.completed.exchange(0, std::memory_order::relaxed), 
				std::memory_order::relaxed
			);
		}
		return *this;
	}

	void Scheduler::Schedule(std::stop_token token) {

		Workers workers;
		TaskGroupMap task_groups;
		std::binary_semaphore resume_handle(0);
		std::mt19937 generator(
			static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count())
		);
		std::atomic_size_t next_id;

		while (!token.stop_requested()) {

			Scheduler* scheduler = nullptr;
			{
				scheduler = s_thread_instance_map[std::this_thread::get_id()];
				scheduler->m_workers.store(&workers, std::memory_order::release);
				scheduler->m_task_groups.store(&task_groups, std::memory_order::release);
				scheduler->m_next_id.store(&next_id, std::memory_order::release);
			}

			if (task_groups.empty() || workers.empty()) {
				scheduler->m_resume_handle.store(&resume_handle, std::memory_order::release);
				resume_handle.acquire();
				continue;
			}

			std::vector<std::size_t> finished_ids;
			std::vector<std::coroutine_handle<>> ready_coroutines;

			{
				/*
				*	look for ready coroutines
				*/

				auto view = task_groups.LockedModifier();
				for (auto const& [id, task_group] : view) {
					if (task_group.completed.load(std::memory_order::acquire) == task_group.total) {
						finished_ids.emplace_back(id);
						ready_coroutines.emplace_back(task_group.coroutine);
					}
				}
			}

			/*
			*  remove ready coroutines and dispatch to other worker threads
			*/

			std::span<std::size_t const> view(finished_ids);
			(void)task_groups.EraseKeys(view);

			std::size_t worker_count = workers.size();
			std::uniform_int_distribution<> int_distribution(0, static_cast<std::uint32_t>(worker_count - 1));

			for (auto& ready_coroutine : ready_coroutines) {
				int random_int = int_distribution(generator);
				workers[random_int].Get()->SubmitTask(
					[ready_coroutine]() {
						ready_coroutine();
					}
				);
			}



		}

	}

	void Scheduler::Notify() noexcept {
		if (std::binary_semaphore* resume_handle = m_resume_handle.exchange(nullptr, std::memory_order::release)) {
			resume_handle->release();
		}
	}

	Scheduler::Workers* Scheduler::LoadWorkers() const noexcept {
		Workers* workers = nullptr;
		do {
			workers = m_workers.load(std::memory_order::acquire);
			if (!workers) {
				std::this_thread::yield();
			}
		} while (!workers);

		return workers;
	}

	Scheduler::TaskGroupMap* Scheduler::LoadTaskGroupMap() const noexcept {
		TaskGroupMap* task_groups = nullptr;
		do {
			task_groups = m_task_groups.load(std::memory_order::acquire);
			if (!task_groups) {
				std::this_thread::yield();
			}
		} while (!task_groups);
		return task_groups;
	}

	std::size_t Scheduler::AcquireNextID() const noexcept {
		std::atomic_size_t* next_id = nullptr;
		do {
			next_id = m_next_id.load(std::memory_order::acquire);
			if (!next_id) {
				std::this_thread::yield();
			}
		} while (!next_id);
		return next_id->fetch_add(1u, std::memory_order::acquire);
	}

	Scheduler::Scheduler()
		: m_scheduler_thread(
			[this](std::stop_token token) {
				s_thread_instance_map[std::this_thread::get_id()] = this;
				Scheduler::Schedule(token);
			}
		) {
	}

	Scheduler::Scheduler(Scheduler&& other) noexcept {
		auto modifier = s_thread_instance_map.LockedModifier();
		for (auto& [_, instance] : modifier) {
			if (instance == &other) {
				instance = this;
			}
		}
	}

	Scheduler::~Scheduler() noexcept {
		if (m_scheduler_thread.joinable()) {
			m_scheduler_thread.request_stop();
			Notify();
		}
	}

	Scheduler& Scheduler::operator=(Scheduler&& other) noexcept {
		
		if (this != &other) {

			auto modifier = s_thread_instance_map.LockedModifier();

			for (auto& [_, instance] : modifier) {
				if (instance == &other) {
					instance = this;
				}
			}

		}

		return *this;

	}

}