module plastic.registrable;

static constexpr std::size_t CacheItem = 10ull;

namespace plastic::utility {

	static std::shared_mutex s_table_mutex;
	static std::unordered_map<std::size_t, void*> s_table;

	namespace details {

		std::size_t plastic::utility::details::IDGen() noexcept {
			static std::atomic_size_t id(1ull);
			return id.fetch_add(1u, std::memory_order::relaxed);
		}

		void RegisterObject(std::size_t id, void* obj) {
			std::lock_guard<std::shared_mutex> lock(s_table_mutex);
			s_table[id] = obj;
		}

		void UnregisterObject(std::size_t id, void* obj) noexcept {

			if (id == 0u) {
				return;
			}

			std::lock_guard<std::shared_mutex> lock(s_table_mutex);
			if (s_table.contains(id) && s_table[id] == obj) {
				s_table.erase(id);
			}

		}

		void* QueryObject(std::size_t id) {
			std::shared_lock<std::shared_mutex> lock(s_table_mutex);
			return s_table.contains(id) ? s_table[id] : nullptr;
		}

	}


}