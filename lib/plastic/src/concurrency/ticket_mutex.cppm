export module plastic.ticket_mutex;
import std;

namespace plastic::concurrency {
	
	export class TicketMutex {
	private:
		alignas(std::hardware_destructive_interference_size) std::atomic_size_t m_next_ticket{ 0 };
		alignas(std::hardware_destructive_interference_size) std::atomic_size_t m_now_serving{ 0 };

	public:
		void lock();
		void unlock();
		bool try_lock();
	};

} // namespace plastic::concurrency
