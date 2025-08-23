export module message_bus;
export import coroutine;
export import thread_pool;
import std;
import defer;
import disable_copy;
import pointer_wrapper;
import concurrent_hash_map;

export namespace util {

	/// @brief Defines a type alias for subscriber identifiers.
	using SubscriberID = std::size_t;

	/// @brief Represents a subscriber that can handle messages of a specific type using a callable object.
	class Subscriber {
	public:
		using CallableVariant = std::variant<
			std::monostate,
			std::function<void(std::any)>, 
			std::function<concurrency::AsyncTask<void>(std::any)>
			>;

		template <class T, class = void>
		struct FunctionTraits {

		};

		template <class Ret, class... Args>
		struct FunctionTraits<Ret(Args...)> {
			using ReturnType = Ret;
			using ArgumentTypes = std::tuple<Args...>;
		};

		template <class Ret, class... Args>
		struct FunctionTraits<Ret(&)(Args...)> : public FunctionTraits<Ret(Args...)> {};

		template <class Ret, class... Args>
		struct FunctionTraits<Ret(*)(Args...)> : public FunctionTraits<Ret(Args...)> {};

		// Specialization for const member function pointers
		template <class R, class C, class... Args>
		struct FunctionTraits<R(C::*)(Args...) const> {
			using ReturnType = R;
			using ArgumentTypes = std::tuple<Args...>;
		};

		// Specialization for non-const member function pointers
		template <class R, class C, class... Args>
		struct FunctionTraits<R(C::*)(Args...)> {
			using ReturnType = R;
			using ArgumentTypes = std::tuple<Args...>;
		};

		// SFINAE specialization for class types with operator()
		template <class Callable>
		struct FunctionTraits<Callable, std::void_t<decltype(&std::remove_cvref_t<Callable>::operator())>> {
			using CallableOperatorType = decltype(&std::remove_cvref_t<Callable>::operator());
			using Traits = FunctionTraits<CallableOperatorType>;
			using ReturnType = typename Traits::ReturnType;
			using ArgumentTypes = typename Traits::ArgumentTypes;
		};

		template <class R, class... Args>
		struct FunctionTraits<std::function<R(Args...)>> {
			using ReturnType = R;
			using ArgumentTypes = std::tuple<Args...>;
		};


	private:
		SubscriberID m_id;
		std::type_info const* m_type;
		CallableVariant m_callable;

	public:
		template <
			class Callable, 
			class Message = std::decay_t<std::tuple_element_t<0, typename FunctionTraits<std::remove_cvref_t<Callable>>::ArgumentTypes>>
		>
		Subscriber(std::size_t id, Callable&& callable)
			: m_id(id),
			m_type(&typeid(Message)) {

			using Traits = FunctionTraits<std::decay_t<std::remove_cvref_t<Callable>>>;
			static_assert(std::tuple_size_v<typename Traits::ArgumentTypes> == 1,
				"Callable must take exactly one argument");

			static constexpr bool IsVoid = std::is_same_v<typename Traits::ReturnType, void>;
			static constexpr bool IsAsyncVoid = std::is_same_v<typename Traits::ReturnType, concurrency::AsyncTask<void>>;

			static_assert(IsVoid || IsAsyncVoid,
				"Return type must be void or concurrency::AsyncTask<void>");

			if constexpr (std::is_same_v<typename Traits::ReturnType, void>) {
				m_callable.emplace<std::function<void(std::any)>>(
					[callable](std::any message) mutable {
						try {
							callable(std::any_cast<Message>(message));
						}
						catch (std::bad_any_cast const& ex) {
							std::cerr << ex.what() << std::endl;
						}
					}
				);
			}
			else {
				m_callable.emplace<std::function<concurrency::AsyncTask<void>(std::any)>>(
					[callable](std::any message) mutable
					-> concurrency::AsyncTask<void> {
						try {
							co_return callable(std::any_cast<Message>(message));
						}
						catch (std::bad_any_cast const& ex) {
							std::cerr << ex.what() << std::endl;
						}
					}
				);
			}

		}

		Subscriber(Subscriber const& other) noexcept = default;
		Subscriber& operator=(Subscriber const& other) noexcept = default;

		Subscriber(Subscriber&& other) noexcept = default;
		Subscriber& operator=(Subscriber&& other) noexcept = default;

		template <class Message>
		void operator()(Message&& message) const {
			std::visit(
				[&message](auto&& callable) {
					using Callable = std::decay_t<decltype(callable)>;
					if constexpr (std::is_same_v<Callable, std::function<void(std::any)>>) {
						callable(std::forward<Message>(message));
					}
					else if constexpr (std::is_same_v<Callable, std::function<concurrency::AsyncTask<void>(std::any)>>) {
						callable(std::forward<Message>(message));
					}
					else {
						// Do nothing
					}
				},
				m_callable
			);
		}

		SubscriberID GetID() const noexcept {
			return m_id;
		}

		std::type_info const& GetType() const noexcept {
			return *m_type;
		}

	};

	/// @brief  The MessageBus class provides a thread-safe publish-subscribe mechanism for message passing between components. 
	///         Subscribers can register callable handlers for specific message types, receive published messages, and be unsubscribed by pointer or ID.
	class MessageBus : public DisableCopy<MessageBus> {
	private:
		using SubscriberMap = concurrency::ConcurrentHashMap<
			SubscriberID,
			Subscriber
		>;

		using MessageRegistryMap = concurrency::ConcurrentHashMap<
			std::type_index,
			SubscriberMap
		>;

		MessageRegistryMap m_message_registries;

		std::atomic<SubscriberID> m_next_id;


	public:
		MessageBus()
			: m_message_registries(),
			m_next_id(0u) {

		}

		MessageBus(MessageBus&& other) noexcept
			: m_message_registries(std::move(other.m_message_registries)),
			m_next_id(m_next_id.exchange(0, std::memory_order::relaxed)) {
		}

		MessageBus& operator=(MessageBus&& other) noexcept {
			if (this != &other) {
				m_message_registries = std::move(other.m_message_registries);
				m_next_id.store(other.m_next_id.load(std::memory_order::relaxed), std::memory_order::relaxed);
			}
			return *this;
		}

		template <class Message, class Callable>
		Subscriber* Subscribe(Callable&& callable) {

			auto id = m_next_id.fetch_add(1u, std::memory_order::acq_rel);

			auto sub_map = m_message_registries[typeid(std::decay_t<Message>)];
			return sub_map.Get().try_emplace(
				id,
				id,
				std::forward<Callable>(callable)
			).first;

		}

		/// @brief Publishes a message to all subscribers registered for its type.
		/// @tparam Message The type of the message to be published. Can be any type.
		/// @param message The message object to be published to subscribers.
		template <class Message>
		void Publish(Message&& message) const {

			try {

				std::vector<Subscriber> subscribers;

				SubscriberMap const& sub_map = m_message_registries.UnsafeAt(typeid(std::decay_t<Message>));
				{
					auto view = sub_map.LockedView();
					for (auto const& [_, subscriber] : view) {
						subscribers.emplace_back(subscriber);
					}
				}

				for (auto const& subscriber : subscribers) {
					subscriber(std::forward<Message>(message));
				}

			}
			catch (std::out_of_range const&) {
				return;
			}

		}

		template <class Message>
		void Publish(Message&& message, concurrency::ThreadPoolPtr const& thread_pool) {

			try {
				SubscriberMap& sub_map = m_message_registries.UnsafeAt(typeid(std::decay_t<Message>));
				auto view = sub_map.LockedView();
				for (auto const& [_, subscriber] : view) {
					thread_pool->SubmitTask(
						[subscriber, message = std::forward<Message>(message)]() {
							subscriber(std::forward<Message>(message));
						}
					);
				}
			}
			catch (std::out_of_range const&) {
				return;
			}

		}

		/// @brief Unsubscribes a message with subscriber registries.
		/// @param subscriber A shared pointer to the subscriber to be unsubscribed.
		void Unsubscribe(Subscriber* subscriber) {
			std::type_index type = subscriber->GetType();
			try {
				SubscriberMap& sub_map = m_message_registries.UnsafeAt(type);
				if (sub_map.erase(subscriber->GetID()) && sub_map.size() == 0) {
					m_message_registries.erase(type);
				}
			}
			catch (std::out_of_range const&) {
				return;
			}
		}

		/// @brief Unsubscribes a messages of a specific type.
		/// @tparam Message The type of message from which will be unsubscribed.
		/// @param id The unique identifier of the subscriber to unsubscribe.
		template <class Message>
		void Unsubscribe(SubscriberID id) {

			std::type_index type = typeid(std::decay_t<Message>);
			try {
				auto sub_map_with_lock = m_message_registries.at(type);
				SubscriberMap& sub_map = sub_map_with_lock;
				if (sub_map.erase(id) && sub_map.size() == 0) {
					m_message_registries.erase(type);
				}
			}
			catch (std::out_of_range const& ex) {
				return;
			}

		}

	};

	using MessageBusPtr = util::PointerWrapper<MessageBus>;

	template <class Message>
	class MessageAwaiter {
	private:
		util::PointerWrapper<util::MessageBus> m_message_bus;
		util::PointerWrapper<concurrency::ThreadPool> m_thread_pool;
		std::exception_ptr m_ex;
		std::function<bool(Message const&)> m_resume_decider;
		std::function<void()> m_release;
		Message m_message;

	public:
		MessageAwaiter(
			util::PointerWrapper<util::MessageBus> const& message_bus,
			util::PointerWrapper<concurrency::ThreadPool> const& thread_pool,
			std::function<bool(Message const&)> const& resume_decider
		) : m_message_bus(message_bus),
			m_thread_pool(thread_pool),
			m_resume_decider(resume_decider) {

		}

		MessageAwaiter(
			util::PointerWrapper<util::MessageBus> const& message_bus,
			std::function<bool(Message const&)> const& resume_decider
		) : m_message_bus(message_bus),
			m_thread_pool(nullptr),
			m_resume_decider(resume_decider) {

		}

		static constexpr bool await_ready() noexcept {
			return false;
		}

		template <class CoroHandle>
		bool await_suspend(CoroHandle suspended_coroutine) noexcept {

			try {
				if constexpr (concurrency::IsTask<CoroHandle>::value) {
					suspended_coroutine.promise().AddReference();
					m_release = [suspended_coroutine]() {
						suspended_coroutine.promise().Release();
						};
				}

				auto id = std::make_shared<util::SubscriberID>();
				*id = m_message_bus->Subscribe<Message>(
					[this, suspended_coroutine, id](Message message) {
						if (m_resume_decider(message)) {
							m_message_bus->Unsubscribe<Message>(*id);
							m_message = std::move(message);
							if (m_thread_pool) {
								m_thread_pool->CommitTask(
									[suspended_coroutine]() {
										suspended_coroutine();
									}
								);
							}
							else {
								suspended_coroutine();
							}
						}
					}
				)->GetID();
				return false;
			}
			catch (std::exception const&) {
				m_ex = std::current_exception();
				return true;
			}
		}

		Message await_resume() {
			util::Defer gc(
				[this]() {
					if (m_release) {
						m_release();
					}
				}
			);

			if (m_ex) {
				std::rethrow_exception(m_ex);
			}

			return std::move(m_message);
		}


	};

}