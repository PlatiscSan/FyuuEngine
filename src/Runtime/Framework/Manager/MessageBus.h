
#pragma once

#ifndef MESSAGE_BUS_H
#define MESSAGE_BUS_H

#include "../Common/Singleton.h"

#include <list>
#include <functional>
#include <typeindex>
#include <unordered_map>

namespace Fyuu {

	class MessageBus : public Singleton<MessageBus> {

		friend class Singleton<MessageBus>;

	public:

		template <class T>
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

		template <class T>
		void Unsubscribe(std::function<void(T&)> callback) {

			std::lock_guard<std::mutex> lock(m_mutex);
			auto type_index = std::type_index(typeid(T));
			if (m_subscriptions.find(type_index) != m_subscriptions.end()) {

				auto& callbacks = m_subscriptions[type_index];
				callbacks.remove_if(
					[&callback](const std::function<void(void*)>& func) {
					return func.target<void(*)(void*)>() == callback.target<void(*)(void*)>();
					}
				);
				if (callbacks.empty()) {
					m_subscriptions.erase(type_index);
				}

			}

		}

		template <class T>
		void Publish(T message) {

			std::lock_guard<std::mutex> lock(m_mutex);
			auto type_index = std::type_index(typeid(T));
			if (m_subscriptions.find(type_index) != m_subscriptions.end()) {
				const auto& callbacks = m_subscriptions[type_index];
				for (const auto& func : callbacks) {
					func(&message); 
				}
			}

		}

	private:

		MessageBus() = default;

	private:

		std::unordered_map<std::type_index, std::list<std::function<void(void*)>>> m_subscriptions;
		std::mutex m_mutex;


	};

}

#endif // !MESSAGE_BUS_H
