module message_bus;
import defer;

namespace fyuu_engine::concurrency {

	void AsyncMessageBus::ProcessMessages(std::stop_token stop_token) {

		MessageQueue message_queue;

		/// used for caching the message published in this thread.
		MessageCache message_cache;
		HandlerMap handler_map;
		std::binary_semaphore resume_handle(0u);

		AsyncMessageBus* instance = nullptr;

		while (!stop_token.stop_requested()) {

			{
				instance = s_thread_instance_map.at(std::this_thread::get_id());

				instance->m_message_queue.store(&message_queue, std::memory_order::release);
				instance->m_message_queue.notify_all();

				instance->m_message_cache.store(&message_cache, std::memory_order::release);
				instance->m_message_cache.notify_all();

				instance->m_handlers.store(&handler_map, std::memory_order::release);
				instance->m_handlers.notify_all();
			}

			if (message_queue.empty() && message_cache.empty()) {
				instance->m_resume_handle.store(&resume_handle, std::memory_order::release);
				/*
				*	wait for messages
				*/
				resume_handle.acquire();
				continue;
			}

			/*
			*  process message queue first
			*/

			while (std::optional<std::any> message = message_queue.TryPopFront()) {
				auto handlers = handler_map.find(message->type());
				if (handlers) {
					/*
					*	invoke callback if associated registration found
					*/
					for (auto const& [_, callback] : *handlers) {
						callback(*message);
					}
				}
			}


			/*
			*	process message cache
			*/

			while (!message_cache.empty()) {
				std::any message = std::move(message_cache.front());
				message_cache.pop_front();
				auto handlers = handler_map.find(message.type());
				if (handlers) {
					/*
					*	invoke callback if associated registration found
					*/
					for (auto const& [_, callback] : *handlers) {
						callback(message);
					}
				}
			}


		}

	}

	AsyncMessageBus::HandlerMap* AsyncMessageBus::LoadHandlerMap() const noexcept {

		HandlerMap* handler_map;

		do {
			handler_map = m_handlers.load(std::memory_order::acquire);
			if (!handler_map) {
				m_handlers.wait(nullptr, std::memory_order::relaxed);
			}
		} while (!handler_map);

		return handler_map;

	}

	AsyncMessageBus::MessageCache* AsyncMessageBus::LoadMessageCache() const noexcept {

		MessageCache* message_cache;

		do {
			message_cache = m_message_cache.load(std::memory_order::acquire);
			if (!message_cache) {
				m_message_cache.wait(nullptr, std::memory_order::relaxed);
			}
		} while (!message_cache);

		return message_cache;

	}

	AsyncMessageBus::MessageQueue* AsyncMessageBus::LoadMessageQueue() const noexcept {

		MessageQueue* message_queue;

		do {
			message_queue = m_message_queue.load(std::memory_order::acquire);
			if (!message_queue) {
				m_message_queue.wait(nullptr, std::memory_order::relaxed);
			}
		} while (!message_queue);

		return message_queue;

	}

	void AsyncMessageBus::Notify() noexcept {
		if (std::binary_semaphore* semaphore = m_resume_handle.exchange(nullptr, std::memory_order::acq_rel)) {
			semaphore->release();
		}
	}

	AsyncMessageBus::AsyncMessageBus()
		: m_message_queue(),
		m_handlers(), 
		m_resume_handle(nullptr),
		m_next_id(0u),
		m_worker(
			[this](std::stop_token stop_token) {
				(void)s_thread_instance_map.try_emplace(std::this_thread::get_id(), this);
				AsyncMessageBus::ProcessMessages(stop_token);
			}
		) {

	}

	AsyncMessageBus::AsyncMessageBus(AsyncMessageBus&& other) noexcept :
		m_next_id(other.m_next_id.exchange(0, std::memory_order::acq_rel)) {

		auto modifier = s_thread_instance_map.LockedModifier();

		for (auto& [id, instance] : modifier) {
			if (instance == &other) {
				instance = this;
			}
		}

		m_worker = std::move(other.m_worker);

	}

	AsyncMessageBus::~AsyncMessageBus() noexcept {
		if (m_worker.joinable()) {
			m_worker.request_stop();
			Notify();
			s_thread_instance_map.erase(m_worker.get_id());
		}
	}

	AsyncMessageBus& AsyncMessageBus::operator=(AsyncMessageBus&& other) noexcept {
		if (this != &other) {

			auto modifier = s_thread_instance_map.LockedModifier();

			for (auto& [id, instance] : modifier) {
				if (instance == &other) {
					instance = this;
				}
			}

			m_next_id = other.m_next_id.exchange(0, std::memory_order::acq_rel);
			m_worker = std::move(other.m_worker);

		}
		return *this;
	}

	SyncMessageBus::SyncMessageBus(SyncMessageBus const& other)
		: m_handlers(other.m_handlers),
		m_next_id(other.m_next_id.load(std::memory_order::acquire)) {
	}

	SyncMessageBus::SyncMessageBus(SyncMessageBus&& other) noexcept
		: m_handlers(std::move(other.m_handlers)),
		m_next_id(other.m_next_id.exchange(0, std::memory_order::acquire)) {
	}

	SyncMessageBus& SyncMessageBus::operator=(SyncMessageBus const& other) {
		if (this != &other) {
			m_handlers = other.m_handlers;
			m_next_id.store(other.m_next_id.load(std::memory_order::acquire), std::memory_order::release);
		}
		return *this;
	}

	SyncMessageBus& SyncMessageBus::operator=(SyncMessageBus&& other) noexcept {
		if (this != &other) {
			m_handlers = std::move(other.m_handlers);
			m_next_id.store(other.m_next_id.exchange(0, std::memory_order::acq_rel), std::memory_order::release);
		}
		return *this;
	}

}