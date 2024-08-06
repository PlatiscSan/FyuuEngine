
#include "pch.h"

#include "MessageBus.h"
#include <memory>

using namespace Fyuu;

static std::mutex s_singleton_mutex;
static std::unique_ptr<MessageBus> s_instance = nullptr;

MessageBus& Fyuu::MessageBus::GetInstance() {
	
	std::lock_guard<std::mutex> lock(s_singleton_mutex);
	if (!s_instance) {
		try {
			s_instance.reset(new MessageBus());
		}
		catch (std::exception const&) {
			throw std::runtime_error("Failed to initialize message bus");
		}
	}
	return *s_instance.get();

}

void Fyuu::MessageBus::DestroyInstance() {

	std::lock_guard<std::mutex> lock(s_singleton_mutex);
	if (s_instance) {
		s_instance.reset();
	}

}
