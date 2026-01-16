module plastic.ticket_mutex;

namespace plastic::concurrency {

	void TicketMutex::lock() {
		
		std::thread::id this_id = std::this_thread::get_id();
		
		// acquire a new ticket

		std::size_t my_ticket = m_next_ticket.fetch_add(1ull, std::memory_order::relaxed);
		std::size_t spin_count = 0;
		std::size_t now_serving;
		do {
			now_serving = m_now_serving.load(std::memory_order::acquire);
			if (now_serving != my_ticket) {
				spin_count++ < 100ull ? 
					std::this_thread::yield() : 
					m_now_serving.wait(now_serving, std::memory_order::relaxed);
			}
		} while (now_serving != my_ticket);
		

	}

	void TicketMutex::unlock() {
		
		/// if the lock is reentrant, just decrease the reentrant count
		
		m_now_serving.fetch_add(1ull, std::memory_order::release);
		m_now_serving.notify_one();

	}

	bool TicketMutex::try_lock() {
		
		// attempt to acquire a new ticket

		std::size_t my_ticket = m_next_ticket.load(std::memory_order::relaxed);
		std::size_t now_serving = m_now_serving.load(std::memory_order::relaxed);
		
		if (my_ticket != now_serving) {
			return false;
		}
		
		if (!m_next_ticket.compare_exchange_strong(my_ticket, my_ticket + 1ull, std::memory_order::acquire, std::memory_order::relaxed)) {
			return false;
		}
		
		return true;

	}

}