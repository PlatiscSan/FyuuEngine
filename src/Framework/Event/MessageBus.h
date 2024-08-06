
#ifndef MESSAGE_BUS_H
#define MESSAGE_BUS_H

#include "Event.h"

#include <concepts>
#include <functional>
#include <list>
#include <mutex>
#include <typeindex>
#include <unordered_map>

namespace Fyuu {

	class MessageBus final {

	public:

		MessageBus(MessageBus const&) = delete;
		MessageBus& operator=(MessageBus const&) = delete;

		MessageBus(MessageBus&&) = delete;
		MessageBus& operator=(MessageBus&&) = delete;

		template <class T> requires std::derived_from<T, CommonEvent>
		void Subscribe(std::function<void(T&)> callback) {
			
			std::lock_guard<std::mutex> lock(m_mutex);
			auto type_index = std::type_index(typeid(T));
			if (m_subscriptions.find(type_index) == m_subscriptions.end()) {
				m_subscriptions[type_index] = std::list<std::function<void(void*)>>();
			}
			m_subscriptions[type_index].emplace_back(
				[callback](void* arg) {
					T* event = static_cast<T*>(arg);
					callback(*event);
				}
			);

		}

		template <class T> requires std::derived_from<T, CommonEvent>
		void Unsubscribe(std::function<void(T&)> callback) {

			std::lock_guard<std::mutex> lock(m_mutex);
			auto type_index = std::type_index(typeid(T));
			if (m_subscriptions.find(type_index) != m_subscriptions.end()) {

				auto& callbacks = m_subscriptions[type_index];
				callbacks.remove_if(
					[&callback](std::function<void(void*)> const& func) {
					return func.target<void(*)(void*)>() == callback.target<void(*)(void*)>();
					}
				);
				if (callbacks.empty()) {
					m_subscriptions.erase(type_index);
				}

			}

		}

		template <class T> requires std::derived_from<T, CommonEvent>
		void Publish(T&& message) {

			std::lock_guard<std::mutex> lock(m_mutex);
			auto type_index = std::type_index(typeid(T));
			if (m_subscriptions.find(type_index) != m_subscriptions.end()) {
				auto const& callbacks = m_subscriptions[type_index];
				for (auto const& func : callbacks) {
					func(&message);
				}
			}

		}

		template <class... T>
		void Publish(T&&... messages) requires (std::derived_from<T, CommonEvent>&&...) {
			if constexpr (sizeof...(messages) > 0) {  
				Publish(messages...);
			} 
		}

		static MessageBus& GetInstance();
		static void DestroyInstance();

	private:

		MessageBus() = default;

		template <class T, class... Args>
		void Publish(T&& first, Args&&... args) requires (std::derived_from<T, CommonEvent> && (std::derived_from<Args, CommonEvent>&&...)) {
			Publish(first);
			if constexpr (sizeof...(Args) > 0) {  
				Publish(args...);
			}
		}

		std::unordered_map<std::type_index, std::list<std::function<void(void*)>>> m_subscriptions;
		std::mutex m_mutex;


	};

}

#endif // !MESSAGE_BUS_H
