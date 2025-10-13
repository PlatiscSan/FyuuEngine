module worker;

namespace fyuu_engine::concurrency {

	void Worker::Work(std::stop_token stop_token) {

		std::binary_semaphore resume_handle(0);
		TaskQueue tasks;
		m_tasks.store(&tasks, std::memory_order::release);

		Worker* instance = nullptr;

		while (!stop_token.stop_requested()) {
			
			{
				instance = s_thread_instance_map.at(std::this_thread::get_id());
				instance->m_tasks.store(&tasks, std::memory_order::release);
			}

			if (tasks.empty()) {
				instance->m_resume_handle.store(&resume_handle, std::memory_order::release);
				resume_handle.acquire();
				continue;
			}

			if (std::optional<Task> task = tasks.TryPopFront()) {
				(*task)();
			}

		}

	}

	void Worker::Notify() noexcept {
		if (std::binary_semaphore* resume_handle = m_resume_handle.exchange(nullptr, std::memory_order_acq_rel)) {
			resume_handle->release();
		}
	}

	Worker::Worker()
		: m_thread(
			[this](std::stop_token stop_token) {
				(void)s_thread_instance_map.try_emplace(std::this_thread::get_id(), this);
				Work(stop_token);
			}
		) {

	}

	Worker::Worker(Worker&& other) noexcept {

		auto modifier = s_thread_instance_map.LockedModifier();

		for (auto& [id, instance] : modifier) {
			if (instance == &other) {
				instance = this;
			}
		}

		m_thread = std::move(other.m_thread);

	}

	Worker::~Worker() noexcept {
		if (m_thread.joinable()) {
			m_thread.request_stop();
			Notify();
			s_thread_instance_map.erase(m_thread.get_id());
		}
	}

	Worker& Worker::operator=(Worker&& other) noexcept {

		if (this != &other) {
			auto modifier = s_thread_instance_map.LockedModifier();

			for (auto& [id, instance] : modifier) {
				if (instance == &other) {
					instance = this;
				}
			}

			m_thread = std::move(other.m_thread);
		}

		return *this;

	}

}