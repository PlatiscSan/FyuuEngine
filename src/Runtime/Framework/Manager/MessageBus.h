
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

		template <class T> void Subscribe(T e, std::function<void(T&)> callback) {

			std::unique_ptr<std::mutex> lock(m_mutex);
			if (m_subscriptions.find(typeIndex) == m_subscriptions.end()) {
				m_subscriptions[typeIndex] = std::list<std::function<void(void*)>>();
			}
			m_subscriptions[typeIndex].emplace_back(
				[callback](void* arg) {
					T* event = static_cast<T*>(arg);
					callback(*event);
				}
			);

		}

	private:

		MessageBus() = default;

	private:

		std::unordered_map<std::type_index, std::list<std::function<void(void*)>>> m_subscriptions;
		std::mutex m_mutex;


	};

}

#endif // !MESSAGE_BUS_H
