export module plastic.event_bus;
import std;
import <nameof.hpp>;
import plastic.unique_resource;

namespace plastic::utility {

	export class EventBus {
	public:
		using Action = std::function<bool(void const*)>;

		class ISubscription 
			: public std::enable_shared_from_this<ISubscription> {
		protected:
			virtual void Unsubscribe(std::size_t sub_id) = 0;

		public:
			class GC {
			private:
				std::weak_ptr<EventBus::ISubscription> m_owner;
			public:
				GC(std::weak_ptr<EventBus::ISubscription> const& owner)
					: m_owner(owner) {

				}

				void operator()(std::size_t sub_id) {
					m_owner.lock()->Unsubscribe(sub_id);
				}
			};

			using SubscriptionHandle = UniqueResource<std::size_t, GC>;

			ISubscription() = default;

			virtual ~ISubscription() noexcept = default;
			virtual void Publish(void const* event) const = 0;
			virtual SubscriptionHandle Subscribe(Action const& action) = 0;
		};

	private:
		template <class Event>
		class DerivedSubscription : public EventBus::ISubscription {
		private:
			using Base = EventBus::ISubscription;

			static inline thread_local std::unordered_map<DerivedSubscription const*, std::size_t> s_publishment_depths;
			static inline thread_local std::deque<std::function<void()>> s_cached_operation;
			
			std::unordered_map<std::size_t, Action> m_actions;
			std::atomic_size_t m_next_id = 0;
			mutable std::shared_mutex m_actions_mutex;

			void Unsubscribe(std::size_t sub_id) override {

				std::size_t& depth = s_publishment_depths[this];

				auto unsubscribe_impl = [this, sub_id]() {
					std::lock_guard<std::shared_mutex> lock(m_actions_mutex);
					m_actions.erase(sub_id);
					};

				if (depth > 0) {
					s_cached_operation.emplace_back(std::move(unsubscribe_impl));
				}
				else {
					unsubscribe_impl();
				}

			}

		public:
			DerivedSubscription()
				: Base() {
			}

			~DerivedSubscription() noexcept override = default;

			void Publish(void const* event) const override {

				std::size_t& depth = s_publishment_depths[this];
				bool const is_recursive_publishment = depth++;
				auto publish_impl = [this, event]() {
					for (auto const& [_, action] : m_actions) {
						bool result = action(event);
						if (!result) {
							break;
						}
					}
					};

				try {
					if (!is_recursive_publishment) {
						std::shared_lock<std::shared_mutex> lock(m_actions_mutex);
						publish_impl();
					}
					else {
						publish_impl();
					}
					--depth;
				}
				catch (...) {
					--depth;
					throw;
				}

				try {
					while (!s_cached_operation.empty()) {
						std::function<void()> func = std::move(s_cached_operation.front());
						s_cached_operation.pop_front();
						func();
					}
				}
				catch (...) {
					s_cached_operation.clear();
					throw;
				}

			}

			SubscriptionHandle Subscribe(Action const& action) override {

				std::size_t& depth = s_publishment_depths[this];
				std::size_t id = m_next_id.fetch_add(1u, std::memory_order::relaxed);

				auto subscribe_impl = [this, action, id]() {
					std::lock_guard<std::shared_mutex> lock(m_actions_mutex);
					m_actions[id] = action;
					};

				if (depth > 0) {
					s_cached_operation.emplace_back(std::move(subscribe_impl));
				}
				else {
					subscribe_impl();
				}

				return SubscriptionHandle(id, this->weak_from_this());

			}

		};

		/// @brief using string to identify the event instead of RTTI
		std::unordered_map<std::string, std::shared_ptr<ISubscription>> m_events;

		mutable std::shared_mutex m_events_mutex;

	public:
		using SubscriptionHandle = ISubscription::SubscriptionHandle;

		template <class Event>
		void Publish(Event&& event) const {

			std::shared_ptr<ISubscription> subscriptions;

			{
				std::shared_lock<std::shared_mutex> lock(m_events_mutex);
				std::string name(NAMEOF_TYPE(Event));
				auto iter = m_events.find(name);
				if (iter == m_events.end()) {
					return;
				}
				subscriptions = iter->second;
			}

			subscriptions->Publish(&event);

		}

		template <class Event, class Action>
		SubscriptionHandle Subscribe(Action&& action) {

			std::shared_ptr<ISubscription> subscriptions;

			{
				std::lock_guard<std::shared_mutex> lock(m_events_mutex);
				std::string name(NAMEOF_TYPE(Event));
				subscriptions = m_events[name];
				if (!subscriptions) {
					subscriptions = std::make_shared<DerivedSubscription<Event>>();
					m_events[name] = subscriptions;
				}
			}

			if constexpr (std::is_convertible_v<std::invoke_result_t<Action, Event>, bool>) {
				return subscriptions->Subscribe(
					[action = std::forward<Action>(action)](void const* event) -> bool {
						return action(*static_cast<Event const*>(event));
					}
				);
			}
			else {
				static_assert(std::is_same_v<std::invoke_result_t<Action, Event>, void>,
					"Action signature does not meet the requirement");
				return subscriptions->Subscribe(
					[action = std::forward<Action>(action)](void const* event) -> bool {
						action(*static_cast<Event const*>(event));
						return true;
					}
				);
			}

		}

	};

}