export module static_hash_map;
export import concurrent_hash_map;
import std;

export namespace fyuu_engine::concurrency {

	template <
		class Key, 
		class T, 
		std::size_t N,
		class Hash = std::hash<Key>,
		class KeyEqual = std::equal_to<Key>
	>
	class StaticHashMap {
	public:
		using key_type = Key;
		using mapped_type = T;
		using value_type = std::pair<ImmutableKey<Key>, T>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using hasher = Hash;
		using key_equal = KeyEqual;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = value_type*;
		using const_pointer = value_type const*;

		class SafeModificationPointer {
			mapped_type* m_mapped;
			std::unique_lock<std::shared_mutex> m_lock;

		public:
			SafeModificationPointer(mapped_type* mapped, std::unique_lock<std::shared_mutex>&& lock)
				: m_mapped(mapped),
				m_lock(std::move(lock)) {
			}

			SafeModificationPointer(std::nullptr_t) noexcept
				:m_mapped(nullptr), m_lock() {

			}

			~SafeModificationPointer() noexcept = default;

			operator bool() const noexcept {
				return m_mapped != nullptr;
			}

			mapped_type* Get() const noexcept {
				return m_mapped;
			}

			operator mapped_type* () const noexcept {
				return m_mapped;
			}

			mapped_type* operator->() const noexcept {
				return m_mapped;
			}

			mapped_type& operator*() {
				if (!m_mapped) {
					throw util::NullPointer();
				}
				return *m_mapped;
			}

			bool operator==(mapped_type* value) const noexcept {
				return m_mapped == value;
			}
		};

		class SafeModificationReference {
		private:
			mapped_type* m_mapped;
			std::unique_lock<std::shared_mutex> m_lock;

		public:
			SafeModificationReference(mapped_type* mapped, std::unique_lock<std::shared_mutex>&& lock)
				: m_mapped(mapped),
				m_lock(std::move(lock)) {
			}

			~SafeModificationReference() noexcept = default;

			void Unlock() {
				m_lock.unlock();
			}

			mapped_type& Get() const noexcept {
				return *m_mapped;
			}

			operator mapped_type& () const noexcept {
				return *m_mapped;
			}

			template <std::convertible_to<mapped_type> CompatibleMappedType>
			SafeModificationReference& operator=(CompatibleMappedType&& value) noexcept {
				*m_mapped = value;
				return *this;
			}

			template <std::convertible_to<mapped_type> CompatibleMappedType>
			bool operator==(CompatibleMappedType&& value) const noexcept {
				return *m_mapped == value;
			}

		};

		class SafeAccessingReference {
		private:
			mapped_type* m_mapped;
			std::shared_lock<std::shared_mutex> m_lock;

		public:
			SafeAccessingReference(mapped_type* mapped, std::shared_lock<std::shared_mutex>&& lock)
				: m_mapped(mapped),
				m_lock(std::move(lock)) {
			}

			~SafeAccessingReference() noexcept = default;

			void Unlock() {
				m_lock.unlock();
			}

			mapped_type& Get() const noexcept {
				return *m_mapped;
			}

			operator mapped_type& () const noexcept {
				return *m_mapped;
			}

			template <std::convertible_to<mapped_type> CompatibleMappedType>
			SafeModificationReference& operator=(CompatibleMappedType&& value) noexcept {
				*m_mapped = value;
				return *this;
			}

			template <std::convertible_to<mapped_type> CompatibleMappedType>
			bool operator==(CompatibleMappedType&& value) const noexcept {
				return *m_mapped == value;
			}

		};

		class AccessingPointer {
		private:
			mapped_type const* m_mapped;
			std::shared_lock<std::shared_mutex> m_lock;

		public:
			AccessingPointer(mapped_type const* mapped, std::shared_lock<std::shared_mutex>&& lock)
				: m_mapped(mapped),
				m_lock(std::move(lock)) {

			}

			AccessingPointer(std::nullptr_t) noexcept
				:m_mapped(nullptr) {

			}

			~AccessingPointer() noexcept = default;

			operator bool() const noexcept {
				return m_mapped != nullptr;
			}

			operator mapped_type const* () const noexcept {
				return m_mapped;
			}

			mapped_type const* operator->() const noexcept {
				return m_mapped;
			}

			mapped_type const& operator*() const {
				if (!m_mapped) {
					throw util::NullPointer();
				}
				return *m_mapped;
			}

			bool operator==(mapped_type* value) const noexcept {
				return m_mapped == value;
			}

		};

	private:
		mutable std::shared_mutex m_mutex;
		std::array<std::optional<value_type>, N> m_elements;
		std::atomic<size_type> m_element_count;
		hasher m_hasher;
		KeyEqual m_key_equal;

		template <class K>
		size_type GetIndex(K const& key) const {
			return m_hasher(key) % N;
		}

		template <class K>
		std::optional<size_type> FindImp(K const& key) const {

			size_type index = GetIndex(key);
			size_type start_index = index;

			do {
				std::optional<value_type> const& element = m_elements[index];
				if (element) {
					if (m_key_equal(element->first, key)) {
						return index;
					}
				}
				index = (index + 1) % N;
			} while (index != start_index);

			return std::nullopt; // Not found
		}

		template <std::convertible_to<value_type> CompatiblePair>
		std::pair<SafeModificationPointer, bool> EmplaceImpl(CompatiblePair&& pair) {
			
			std::unique_lock<std::shared_mutex> lock(m_mutex);

			// Check if key already exists

			std::optional<size_type> existing_index = FindImp(pair.first);
			if (existing_index) {
				return {
					std::piecewise_construct,
					std::forward_as_tuple(&m_elements[*existing_index]->second, std::move(lock)),
					std::forward_as_tuple(false)
				};
			}

			// Find empty slot

			size_type index = GetIndex(pair.first);
			size_type start_index = index;

			do {

				std::optional<value_type>& element = m_elements[index];

				if (!element) {
					element.emplace(std::forward<CompatiblePair>(pair));
					m_element_count.fetch_add(1, std::memory_order_release);
					return {
						std::piecewise_construct,
						std::forward_as_tuple(&m_elements[index]->second, std::move(lock)),
						std::forward_as_tuple(true)
					};
				}
				index = (index + 1) % N;
			} while (index != start_index);

			// Table is full
			return { nullptr, false };

		}

		template <class K, class... Args>
		std::pair<SafeModificationPointer, bool> EmplaceImpl(K&& key, Args&&... args) {

			std::unique_lock<std::shared_mutex> lock(m_mutex);

			// Check if key already exists

			std::optional<size_type> existing_index = FindImp(std::forward<K>(key));
			if (existing_index) {
				return {
					std::piecewise_construct,
					std::forward_as_tuple(&m_elements[*existing_index]->second, std::move(lock)),
					std::forward_as_tuple(false)
				};
			}

			// Find empty slot

			size_type index = GetIndex(key);
			size_type start_index = index;

			do {

				std::optional<value_type>& element = m_elements[index];

				if (!element) {
					m_elements[index].emplace(
						std::piecewise_construct,
						std::forward_as_tuple(std::forward<K>(key)),
						std::forward_as_tuple(std::forward<Args>(args)...)
					);
					m_element_count.fetch_add(1, std::memory_order_release);
					return {
						std::piecewise_construct,
						std::forward_as_tuple(&m_elements[index]->second, std::move(lock)),
						std::forward_as_tuple(true)
					};
				}
				index = (index + 1) % N;
			} while (index != start_index);

			// Table is full
			return { nullptr, false };

		}

		std::unique_lock<std::shared_mutex> ClearImp() noexcept {

			std::unique_lock<std::shared_mutex> lock(m_mutex);

			for (std::optional<value_type>& element : m_elements) {
				element.reset();
				m_element_count.fetch_sub(1u, std::memory_order::release);
			}

			return lock;

		}

	public:
		StaticHashMap(
			Hash const& hash = Hash(),
			key_equal const& equal = key_equal()
		) : m_hasher(hash), m_key_equal(equal) {

		}

		StaticHashMap(
			std::initializer_list<value_type> init,
			Hash const& hash = Hash(),
			key_equal const& equal = key_equal()
		) : StaticHashMap(hash, equal) {

			for (auto& value : init) {
				EmplaceImpl(value);
			}

		}

		SafeModificationReference at(Key const& key) {

			std::unique_lock<std::shared_mutex> lock(m_mutex);
			std::optional<size_type> index = FindImp(key);

			if (index) {
				return SafeModificationReference(&m_elements[*index]->second, std::move(lock));
			}

			throw std::out_of_range("the key does not exist in the hash table");

		}

		SafeAccessingReference at(Key const& key) const {

			std::shared_lock<std::shared_mutex> lock(m_mutex);
			std::optional<size_type> index = FindImp(key);

			if (index) {
				return SafeAccessingReference(&(*m_elements[*index]), std::move(lock));
			}

			throw std::out_of_range("the key does not exist in the hash table");

		}

		T& UnsafeAt(Key const& key) {

			std::optional<size_type> index = FindImp(key);

			if (index) {
				return m_elements[*index]->second;
			}

			throw std::out_of_range("the key does not exist in the hash table");

		}

		T const& UnsafeAt(Key const& key) const {

			std::optional<size_type> index = FindImp(key);

			if (index) {
				return m_elements[*index]->second;
			}

			throw std::out_of_range("the key does not exist in the hash table");

		}

		SafeModificationReference operator[](Key const& key) {

			std::unique_lock<std::shared_mutex> lock(m_mutex);
			std::optional<size_type> index = FindImp(key);

			if (index) {
				return SafeModificationReference(&(*m_elements[*index]), std::move(lock));
			}

			auto [safe_ptr, success] = EmplaceImpl(key, T{});
			if (!success) {
				throw std::runtime_error("StaticHashMap is full");
			}

			return SafeModificationReference(safe_ptr.Get(), std::move(lock));

		}

		std::pair<SafeModificationPointer, bool> insert(value_type const& value) {
			return EmplaceImpl(value);
		}

		std::pair<SafeModificationPointer, bool> insert(value_type&& value) {
			return EmplaceImpl(value);
		}

		template <class... Args>
		std::pair<SafeModificationPointer, bool> emplace(Args&&... args) {
			return EmplaceImpl(std::forward<Args>(args)...);
		}

		template <class K, class... Args>
		std::pair<SafeModificationPointer, bool> try_emplace(K&& key, Args&&... args) {
			return EmplaceImpl(std::forward<K>(key), std::forward<Args>(args)...);
		}

		void clear() noexcept {
			(void)ClearImp();
		}

		AccessingPointer find(Key const& key) const {

			std::shared_lock<std::shared_mutex> lock(m_mutex);

			if (m_element_count.load(std::memory_order::acquire) == 0) {
				return nullptr;
			}

			for (auto const& [first, second] : m_elements) {
				if (m_key_equal(*first, key)) {
					return { &second, std::move(lock) };
				}
			}

			return nullptr;

		}

		SafeModificationPointer find(Key const& key) {
			
			std::unique_lock<std::mutex> lock(m_mutex);

			if (m_element_count.load(std::memory_order::acquire) == 0) {
				return nullptr;
			}

			for (auto& [first, second] : m_elements) {
				if (m_key_equal(*first, key)) {
					return { &second, std::move(lock) };
				}
			}

			return nullptr;
		}

		bool erase(Key const& key) {

			std::unique_lock<std::shared_mutex> lock(m_mutex);

			size_type index = GetIndex(key);
			size_type start_index = index;

			do {

				std::optional<value_type>& element = m_elements[index];

				if (m_key_equal(*element, key)) {
					element.reset();
					return true;
				}

			} while (start_index != index);

			return false;

		}

	};

}