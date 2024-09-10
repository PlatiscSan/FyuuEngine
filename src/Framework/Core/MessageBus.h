#ifndef MESSAGE_BUS_H
#define MESSAGE_BUS_H

#include <any>
#include <chrono>
#include <functional>

namespace Fyuu::core {

	class BaseEvent {

	public:

		virtual ~BaseEvent() noexcept = default;

	protected:

		BaseEvent() = default;

		std::chrono::system_clock::time_point const m_timestamp = std::chrono::system_clock::now();

	};

}

namespace Fyuu::core::message_bus {

	using CallbackID = std::size_t;

	CallbackID Subscribe(std::type_index const& type_index, std::function<void(std::any const&)> func);
	void Unsubscribe(std::type_index const& type_index, CallbackID id);
	void Publish(std::type_index const& type_index, std::any const& arg);
	void ClearSubscriptions();

	template <class T> requires std::derived_from<T, BaseEvent>
	CallbackID Subscribe(std::function<void(T&)> func) {
		return Subscribe(
			typeid(T),
			[func](std::any const& arg) {
				auto _arg = std::any_cast<T>(arg);
				func(_arg);
			}
		);
	}

	template <class T> requires std::derived_from<T, BaseEvent>
	void Unsubscribe(CallbackID id) {
		Unsubscribe(typeid(T), id);
	}

	template <class T> requires std::derived_from<T, BaseEvent>
	void Publish(T&& e) {
		Publish(typeid(e), e);
	}


}

#endif // !MESSAGE_BUS_H
