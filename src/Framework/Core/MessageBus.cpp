
#include "pch.h"
#include "MessageBus.h"

using namespace Fyuu::core::message_bus;

static std::size_t s_next_id = 0;

struct Callback {
	std::function<void(std::any const&)> func;
	std::size_t const id = s_next_id++;
};

static std::unordered_map<std::type_index, std::list<Callback>> s_subscriptions;
static std::mutex s_mutex;

CallbackID Fyuu::core::message_bus::Subscribe(std::type_index const& type_index, std::function<void(std::any const&)> func) {

	std::lock_guard<std::mutex> lock(s_mutex);
	if (s_subscriptions.find(type_index) == s_subscriptions.end()) {
		s_subscriptions.emplace(type_index, std::list<Callback>());
	}
	Callback callback = { .func = func };
	s_subscriptions[type_index].emplace_back(callback);
	return callback.id;

}

void Fyuu::core::message_bus::Unsubscribe(std::type_index const& type_index, CallbackID id) {

	std::lock_guard<std::mutex> lock(s_mutex);
	if (s_subscriptions.find(type_index) != s_subscriptions.end()) {

		auto& callbacks = s_subscriptions[type_index];
		callbacks.remove_if([&id](Callback const& callback) {return callback.id == id; });
		if (callbacks.empty()) {
			s_subscriptions.erase(type_index);
		}
	}

}

void Fyuu::core::message_bus::Publish(std::type_index const& type_index, std::any const& arg) {

	std::lock_guard<std::mutex> lock(s_mutex);
	if (s_subscriptions.find(type_index) != s_subscriptions.end()) {
		auto const& callbacks = s_subscriptions[type_index];
		for (auto const& callback : callbacks) {
			callback.func(arg);
		}
	}

}

void Fyuu::core::message_bus::ClearSubscriptions() {

	std::lock_guard<std::mutex> lock(s_mutex);
	s_subscriptions.clear();

}
