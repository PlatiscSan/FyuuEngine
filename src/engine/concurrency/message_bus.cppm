export module message_bus;
import std;
import circular_buffer;
import concurrent_hash_map;
import disable_copy;
import pointer_wrapper;

namespace fyuu_engine::concurrency {

	export class ISubscription : public util::DisableCopy<ISubscription> {
	public:
		virtual ~ISubscription() = default;
		virtual void Unsubscribe() = 0;

	};

	export class AsyncMessageBus final 
		: public util::EnableSharedFromThis<AsyncMessageBus> {
	private:
		using MessageQueue = CircularBuffer<std::any, 32u>;
		using MessageCache = std::deque<std::any>;
		using HandlerMap = ConcurrentHashMap<std::type_index, std::unordered_map<std::size_t, std::function<void(std::any)>>>;

		std::atomic<MessageQueue*> m_message_queue;
		std::atomic<HandlerMap*> m_handlers;
		std::atomic<std::binary_semaphore*> m_resume_handle;

		/// @brief used for message publishment cache in worker thread.
		std::atomic<MessageCache*> m_message_cache;
		std::atomic_size_t m_next_id;
		std::jthread m_worker;

		/// @brief one worker thread can only be associated to one message bus instance.
		static inline concurrency::ConcurrentHashMap<std::thread::id, AsyncMessageBus*> s_thread_instance_map;

		template <class T>
		class Subscription : public ISubscription {
		private:
			util::PointerWrapper<AsyncMessageBus> m_bus;
			std::size_t m_id;
			std::type_info const& m_type;

		public:
			Subscription(util::PointerWrapper<AsyncMessageBus> const& bus, std::size_t id)
				: m_bus(util::MakeReferred(bus, false)), m_id(id), m_type(typeid(std::decay_t<T>)) {
			}

			~Subscription() noexcept override {
				Unsubscribe();
			}

			void Unsubscribe() override {
				AsyncMessageBus* raw = m_bus.Get();

				if (!raw->m_worker.joinable()) {
					return;
				}

				HandlerMap* handler_map = raw->LoadHandlerMap();
				auto handlers = handler_map->find(m_type);
				handlers->erase(m_id);
			}


		};

		static void ProcessMessages(std::stop_token token);

		HandlerMap* LoadHandlerMap() const noexcept;
		MessageCache* LoadMessageCache() const noexcept;
		MessageQueue* LoadMessageQueue() const noexcept;

		void Notify() noexcept;

		template <class T, class... Args>
		void WorkerThreadPublish(Args&&... args) const {
			MessageCache* message_cache = LoadMessageCache();
			message_cache->emplace_back(std::in_place_type<T>, std::forward<Args>(args)...);
			Notify();
		}

		template <class T, class... Args>
		void PublishImp(Args&&... args) const {
			MessageQueue* message_queue = LoadMessageQueue();
			message_queue->emplace_back(std::in_place_type<T>, std::forward<Args>(args)...);
			Notify();
		}

	public:
		AsyncMessageBus();
		AsyncMessageBus(AsyncMessageBus&& other) noexcept;
		~AsyncMessageBus() noexcept;

		AsyncMessageBus& operator=(AsyncMessageBus&& other) noexcept;

		template <class T, class... Args>
		void Publish(Args&&... args) const {

			if (!m_worker.joinable()) {
				return;
			}

			m_worker.get_id() == std::this_thread::get_id() ?
				WorkerThreadPublish<T>(std::forward<Args>(args)...) :
				PublishImp<T>(std::forward<Args>(args)...);

		}

		template <class T, class Func>
		std::shared_ptr<ISubscription> Subscribe(Func&& callback) {

			if (!m_worker.joinable()) {
				return nullptr;
			}

			HandlerMap* handlers = LoadHandlerMap();

			std::type_info const& type = typeid(std::decay_t<T>);
			std::size_t id = m_next_id.fetch_add(1u, std::memory_order::acq_rel);
			(void)(*handlers)[type].Get().try_emplace(
				id, 
				[callback](std::any const& message) {

					try {
						if constexpr (std::is_invocable_v<Func, std::decay_t<T>>) {
							callback(std::any_cast<std::decay_t<T>>(message));
						}
						else {
							callback();
						}
					}
					catch (std::bad_any_cast const&) {
						// ignore bad cast
					}
				}
			);

			std::shared_ptr<ISubscription> subscription 
				= std::make_shared<Subscription<T>>(This(), id);

			return subscription;
		}

	};

	export class SyncMessageBus final :
		public util::EnableSharedFromThis<SyncMessageBus> {
	private:
		using HandlerMap = ConcurrentHashMap<std::type_index, std::unordered_map<std::size_t, std::function<void(std::any)>>>;

		HandlerMap m_handlers;
		std::atomic_size_t m_next_id;

		template <class T>
		class Subscription : public ISubscription {
		private:
			util::PointerWrapper<SyncMessageBus> m_bus;
			std::size_t m_id;
			std::type_info const& m_type;

		public:
			Subscription(util::PointerWrapper<SyncMessageBus> const& bus, std::size_t id)
				: m_bus(util::MakeReferred(bus, false)), m_id(id), m_type(typeid(std::decay_t<T>)) {
			}

			~Subscription() noexcept override {
				Unsubscribe();
			}

			void Unsubscribe() override {
				SyncMessageBus* raw = m_bus.Get();

				auto handlers = m_bus->m_handlers.find(m_type);
				handlers->erase(m_id);
			}


		};

	public:
		SyncMessageBus() = default;
		SyncMessageBus(SyncMessageBus const& other);
		SyncMessageBus(SyncMessageBus&& other) noexcept;
		~SyncMessageBus() noexcept = default;

		SyncMessageBus& operator=(SyncMessageBus const& other);
		SyncMessageBus& operator=(SyncMessageBus&& other) noexcept;

		template <class T, class... Args>
		void Publish(Args&&... args) const try {

			std::vector<std::function<void(std::any)>> callbacks;
			std::type_info const& type = typeid(std::decay_t<T>);

			{
				auto safe_subscription_map = m_handlers.find(type);

				if (!safe_subscription_map) {
					return;
				}

				callbacks.reserve(safe_subscription_map->size());
				for (auto& [_, callback] : *safe_subscription_map) {
					callbacks.emplace_back(callback);
				}
			}

			std::decay_t<T> message(std::forward<Args>(args)...);

			for (auto const& callback : callbacks) {
				callback(message);
			}

		}
		catch (std::out_of_range const&) {
			return;
		}

		template <class T, class Func>
		std::shared_ptr<ISubscription> Subscribe(Func&& callback) {

			std::type_index type = typeid(std::decay_t<T>);
			std::size_t id = m_next_id.fetch_add(1u, std::memory_order::acq_rel);
			(void)m_handlers[type].Get().try_emplace(
				id,
				[callback](std::any const& message) {

					try {
						if constexpr (std::is_invocable_v<Func, std::decay_t<T>>) {
							callback(std::any_cast<std::decay_t<T>>(message));
						}
						else {
							callback();
						}
					}
					catch (std::bad_any_cast const&) {
						// ignore bad cast
					}
				}
			);

			std::shared_ptr<ISubscription> subscription
				= std::make_shared<Subscription<T>>(This(), id);

			return subscription;
		}

	};

}