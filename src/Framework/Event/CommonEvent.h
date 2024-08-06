#ifndef COMMON_EVENT_H
#define COMMON_EVENT_H

#include <chrono>

namespace Fyuu {

	class CommonEvent {

	public:

		auto const& GetTimestamp() const noexcept {
			return m_timestamp;
		}
		virtual ~CommonEvent() = default;

	private:

		std::chrono::system_clock::time_point const m_timestamp = std::chrono::system_clock::now();

	};

}

#endif // !COMMON_EVENT_H
