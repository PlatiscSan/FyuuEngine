export module thread_cache;
import std;
import circular_buffer;

namespace fyuu_engine::concurrency {

	template <class T, std::size_t N, class Hash = std::hash<std::thread::id>>
	class ThreadCache {
	private:
		struct Slot {
			CircularBuffer<T, N, true> buffer;
			std::thread::id owner_id;
		};

		static constexpr size_type SlotCount = std::hardware_destructive_interference_size;

		Hash m_hash;
		std::array<Slot, SlotCount> slots;


	public:

	};

}