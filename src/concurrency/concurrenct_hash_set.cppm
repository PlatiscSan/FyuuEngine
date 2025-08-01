export module concurrent_hash_set;
export import concurrent_container_base;
import std;

export namespace concurrency {

	template<
		class Key,
		class Hash = std::hash<Key>,
		class KeyEqual = std::equal_to<Key>,
		class Allocator = std::allocator<Key>
	> class ConcurrentHashSet {
	public:
		using key_type = Key;
		using value_type = Key;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using hasher = Hash;
		using key_equal = KeyEqual;
		using allocator_type = Allocator;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = typename std::allocator_traits<Allocator>::pointer;
		using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

	private:
		using Keys = std::vector<value_type, allocator_type>;

		struct Bucket {
			Keys keys;
			mutable Mutex mutex;

			Bucket(allocator_type const& alloc)
				: keys(alloc),
				mutex() {

			}

			Bucket(Bucket const& other)
				: keys(other.keys.get_allocator()) {
				auto other_lock = other.mutex.LockShared();
				for (auto const& pair : other.keys) {
					keys.emplace_back(pair);
				}
			}

			Bucket& operator=(Bucket const& other) {
				if (this != &other) {

					auto lock = other.mutex.LockShared();
					auto this_lock = mutex.Lock();

					for (auto const& pair : other.keys) {
						keys.emplace_back(pair);
					}

				}
				return *this;
			}

			Bucket(Bucket&& other) noexcept
				: keys(other.keys.get_allocator()) {
				auto lock = other.mutex.Lock();
				keys = std::move(other.keys);
			}

			Bucket& operator=(Bucket&& other) noexcept {
				if (this != &other) {
					auto other_lock = other.mutex.Lock();
					auto this_lock = mutex.Lock();
					keys = std::move(other.keys);
				}
				return *this;
			}

			void Clear() {
				auto lock = mutex.Lock();
				keys.clear();
			}

		};

		using BucketAlloc = typename std::allocator_traits<allocator_type>::template rebind_alloc<Bucket>;
		using BucketAllocTraits = std::allocator_traits<BucketAlloc>;

		using Buckets = std::vector<Bucket, BucketAlloc>;

		using UniqueLockAlloc = typename std::allocator_traits<allocator_type>::template rebind_alloc<UniqueLock>;
		using SharedLockAlloc = typename std::allocator_traits<allocator_type>::template rebind_alloc<SharedLock>;
		using UniqueLocks = std::vector<std::unique_lock<std::shared_mutex>, UniqueLockAlloc>;
		using SharedLocks = std::vector<std::shared_lock<std::shared_mutex>, SharedLockAlloc>;

		Buckets m_buckets;
		mutable Mutex m_buckets_mutex;

		std::atomic<size_type> m_size;
		float m_max_load_factor;
		hasher m_hasher;
		key_equal m_key_equal;

	public:
		template <class Container, class Value, class ElementIterator>
		class IteratorBase {
		protected:
			Container* m_map;
			size_type m_bucket_idx;
			ElementIterator m_element_iter;

		private:
			void MakeEndIterator() {
				if (m_map->m_buckets.empty()) {
					throw EmptyContainer();
				}
				else {
					// Point to the end iterator of the last bucket
					m_bucket_idx = m_map->m_buckets.size() - 1;
					m_element_iter = m_map->m_buckets.back().keys.end();
				}
			}

			void SeekToLastElement() {
				m_bucket_idx = m_map->m_buckets.size() - 1;
				while (m_bucket_idx > 0 &&
					m_map->m_buckets[m_bucket_idx].keys.empty()) {
					--m_bucket_idx;
				}

				auto& last_bucket = m_map->m_buckets[m_bucket_idx];
				m_element_iter = --last_bucket.keys.end();

			}

			void SeekToPreviousBucket() {

				size_type original_bucket = m_bucket_idx;

				while (m_bucket_idx > 0) {
					--m_bucket_idx;
					auto& bucket = m_map->m_buckets[m_bucket_idx];
					if (!bucket.keys.empty()) {
						m_element_iter = --bucket.keys.end();
						return;
					}
				}

				MakeEndIterator();

			}

			bool IsEnd() const noexcept {
				return
					m_bucket_idx == m_map->m_buckets.size() - 1 &&
					m_element_iter == m_map->m_buckets.back().keys.end();
			}

		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using value_type = Value;
			using difference_type = std::ptrdiff_t;
			using pointer = value_type*;
			using reference = value_type&;

			IteratorBase() noexcept : m_map(nullptr), m_bucket_idx(0) {}

			IteratorBase(ConcurrentHashSet* map, size_type bucket_idx,
				ElementIterator element_iter) noexcept
				: m_map(map), m_bucket_idx(bucket_idx),
				m_element_iter(element_iter) {
			}

			template <class = typename std::enable_if_t<!std::is_const_v<Container>>>
			reference operator*() noexcept {
				return *m_element_iter;
			}

			const_reference operator*() const noexcept {
				return *m_element_iter;
			}

			template <class = typename std::enable_if_t<!std::is_const_v<Container>>>
			pointer operator->() noexcept {
				return &(*m_element_iter);
			}

			const_pointer operator->() const noexcept {
				return &(*m_element_iter);
			}

			IteratorBase& operator++() {

				if (IsEnd()) {
					return *this;
				}

				if (++m_element_iter != m_map->m_buckets[m_bucket_idx].keys.end()) {
					return *this;
				}

				while (++m_bucket_idx < m_map->m_buckets.size()) {
					auto& bucket = m_map->m_buckets[m_bucket_idx];
					if (!bucket.keys.empty()) {
						m_element_iter = bucket.keys.begin();
						return *this;
					}
				}

				MakeEndIterator();
				return *this;

			}

			IteratorBase operator++(int) {
				IteratorBase tmp = *this;
				++(*this);
				return tmp;
			}

			IteratorBase& operator--() {

				if (IsEnd()) {
					if (m_map->empty()) {
						return *this;
					}
					SeekToLastElement();
					return *this;
				}

				if (m_element_iter-- != m_map->m_buckets[m_bucket_idx].keys.begin()) {
					return *this;
				}

				SeekToPreviousBucket();
				return*this;

			}

			IteratorBase operator--(int) {
				IteratorBase tmp = *this;
				--(*this);
				return tmp;
			}

			bool operator==(IteratorBase const& other) const noexcept {

				if (m_map != other.m_map) {
					return false;
				}

				if (IsEnd() && other.IsEnd()) {
					return true;
				}

				if (IsEnd() || other.IsEnd()) {
					return false;
				}

				return m_bucket_idx == other.m_bucket_idx &&
					m_element_iter == other.m_element_iter;
			}

			bool operator!=(IteratorBase const& other) const noexcept {
				return !(*this == other);
			}

		};

		class iterator : public IteratorBase<ConcurrentHashSet, value_type, typename Keys::iterator> {
		public:
			using Base = IteratorBase<ConcurrentHashSet, value_type, typename Keys::iterator>;
			using Base::Base;

		};

		class const_iterator : public IteratorBase<ConcurrentHashSet const, value_type const, typename Keys::const_iterator> {
		public:
			using Base = IteratorBase<ConcurrentHashSet const, value_type const, typename Keys::const_iterator>;
			using Base::Base;

			const_iterator(iterator const& other) noexcept
				: Base(other.m_map, other.m_bucket_idx, other.m_element_iter) {
			}
		};

		/// @brief Provides exclusive, thread-safe access to all elements of a ConcurrentHashSet for modification or iteration.
		class Modifier {
		private:
			ConcurrentHashSet* m_map;
			UniqueLocks m_locks;

		public:
			explicit Modifier(ConcurrentHashSet* map)
				: m_map(map), m_locks(m_map->m_buckets.get_allocator()) {
				m_locks.emplace_back(m_map->m_buckets_mutex.Lock());

				size_type buckets_count = m_map->m_buckets.size();

				for (size_type i = 0u; i < buckets_count; ++i) {
					m_locks.emplace_back(m_map->m_buckets[i].mutex.Lock());
				}

			}

			~Modifier() noexcept = default;

			/// @brief Returns an iterator to the end of the container.
			/// @return An iterator pointing past the last element of the container. Throws EmptyContainer if the container is empty.
			iterator end() {

				size_type bucket_count = m_map->m_buckets.size();
				if (bucket_count != 0) {
					size_type last_bucket = bucket_count - 1;
					return iterator(m_map, last_bucket, m_map->m_buckets[last_bucket].keys.end());
				}

				throw EmptyContainer();

			}

			/// @brief Returns an iterator to the first element in the container.
			/// @return An iterator pointing to the first element in the container, or to end() if the container is empty.
			iterator begin() {
				for (size_type i = 0; i < m_map->m_buckets.size(); ++i) {
					if (!m_map->m_buckets[i].keys.empty()) {
						return iterator(m_map, i, m_map->m_buckets[i].keys.begin());
					}
				}
				return end();
			}

			/// @brief Returns a constant iterator to the end of the container.
			/// @return A const_iterator pointing past the last element of the container.
			const_iterator cend() const {
				size_type bucket_count = m_map->m_buckets.size();
				if (bucket_count != 0) {
					size_type last_bucket = bucket_count - 1;
					return const_iterator(m_map, last_bucket, m_map->m_buckets[last_bucket].keys.end());
				}

				throw EmptyContainer();
			}

			/// @brief Returns a constant iterator to the first element in the container.
			/// @return A constant iterator pointing to the first element in the container, or to cend() if the container is empty.
			const_iterator cbegin() const {
				for (size_type i = 0; i < m_map->m_buckets.size(); ++i) {
					if (!m_map->m_buckets[i].empty()) {
						return const_iterator(m_map, i, m_map->m_buckets[i].cbegin());
					}
				}
				return cend();
			}

			/// @brief Returns a constant iterator to the beginning of the container.
			/// @return A constant iterator pointing to the first element of the container.
			const_iterator begin() const {
				return cbegin();
			}

			/// @brief Returns a constant iterator to the end of the container.
			/// @return A constant iterator pointing past the last element of the container.
			const_iterator end() const {
				return cend();
			}

		};

		/// @brief Provides a read-only view over the contents of a ConcurrentHashSet, acquiring shared locks for thread-safe iteration.
		class View {
		private:
			ConcurrentHashSet* m_map;
			SharedLocks m_shared_locks;

		public:
			View(ConcurrentHashSet* map)
				: m_map(map), m_shared_locks(m_map->m_buckets.get_allocator()) {

				m_shared_locks.emplace_back(m_map->m_buckets_mutex.LockShared());

				size_type buckets_count = m_map->m_buckets.size();

				for (size_type i = 0u; i < buckets_count; ++i) {
					m_shared_locks.emplace_back(m_map->m_buckets[i].mutex.LockShared());
				}

			}

			~View() noexcept = default;

			/// @brief Returns a const iterator to the end of the container.
			/// @return A const_iterator pointing past the last element of the container. Throws EmptyContainer if the container is empty.
			const_iterator end() const {

				size_type bucket_count = m_map->m_buckets.size();
				if (bucket_count != 0) {
					size_type last_bucket = bucket_count - 1;
					return const_iterator(m_map, last_bucket, m_map->m_buckets[last_bucket].keys.end());
				}

				throw EmptyContainer();

			}

			/// @brief Returns a constant iterator to the first element in the container.
			/// @return A const_iterator pointing to the first element in the container, or to end() if the container is empty.
			const_iterator begin() const {
				for (size_type i = 0; i < m_map->m_buckets.size(); ++i) {
					if (!m_map->m_buckets[i].keys.empty()) {
						return const_iterator(m_map, i, m_map->m_buckets[i].keys.cbegin());
					}
				}
				return end();
			}

			/// @brief Returns a constant iterator to the beginning of the container.
			/// @return A constant iterator pointing to the first element of the container.
			const_iterator cbegin() const {
				return begin();
			}

			/// @brief Returns a constant iterator to the end of the container.
			/// @return A constant iterator pointing past the last element of the container.
			const_iterator cend() const {
				return end();
			}

		};


		/// @brief A smart pointer-like class that provides read-only access to a key object while holding a shared lock for thread safety.
		class AccessingPointer {
		private:
			key_type const* m_key;
			std::shared_lock<std::shared_mutex> m_lock;

		public:
			AccessingPointer(key_type const* key, std::shared_lock<std::shared_mutex>&& lock)
				: m_key(key),
				m_lock(std::move(lock)) {

			}

			AccessingPointer(std::nullptr_t) noexcept
				:m_key(nullptr) {

			}

			~AccessingPointer() noexcept = default;

			operator bool() const noexcept {
				return m_key != nullptr;
			}

			operator key_type const* () const noexcept {
				return m_key;
			}

			key_type const* operator->() const noexcept {
				return m_key;
			}

			key_type const& operator*() const {
				if (!m_key) {
					throw util::NullPointer();
				}
				return *m_key;
			}

			bool operator==(key_type* value) const noexcept {
				return m_key == value;
			}

		};

		/// @brief A smart pointer-like class that manages exclusive access to a key value within a bucket, ensuring thread safety via a unique lock.
		class BucketModificationPointer {
		private:
			key_type* m_key;
			std::unique_lock<std::shared_mutex> m_bucket_lock;

		public:
			BucketModificationPointer(key_type* key, std::unique_lock<std::shared_mutex>&& bucket_lock)
				: m_key(key),
				m_bucket_lock(std::move(bucket_lock)) {
			}

			BucketModificationPointer(std::nullptr_t) noexcept
				:m_key(nullptr), m_bucket_lock() {

			}

			~BucketModificationPointer() noexcept = default;

			operator bool() const noexcept {
				return m_key != nullptr;
			}

			operator key_type* () const noexcept {
				return m_key;
			}

			key_type* operator->() const noexcept {
				return m_key;
			}

			key_type& operator*() {
				if (!m_key) {
					throw util::NullPointer();
				}
				return *m_key;
			}

			bool operator==(key_type* value) const noexcept {
				return m_key == value;
			}

		};

		/// @brief Provides a reference-like interface for modifying a value in a bucket, ensuring thread-safe access via a unique lock.
		class BucketModificationReference {
		private:
			key_type* m_key;
			std::unique_lock<std::shared_mutex> m_bucket_lock;

		public:
			BucketModificationReference(key_type* key, std::unique_lock<std::shared_mutex>&& bucket_lock)
				: m_key(key),
				m_bucket_lock(std::move(bucket_lock)) {
			}

			~BucketModificationReference() noexcept = default;

			void Unlock() {
				m_bucket_lock.unlock();
			}

			operator key_type& () const noexcept {
				return *m_key;
			}

			template <std::convertible_to<key_type> CompatibleMappedType>
			BucketModificationReference& operator=(CompatibleMappedType&& value) noexcept {
				*m_key = value;
				return *this;
			}

			template <std::convertible_to<key_type> CompatibleMappedType>
			bool operator==(CompatibleMappedType&& value) const noexcept {
				return *m_key == value;
			}

		};

	private:
		template <std::convertible_to<Key> K>
		size_type BucketIndex(K&& key) noexcept {
			return m_hasher(key) % m_buckets.size();
		}

		/// @brief Determines whether the container should be rehashed based on its current load factor.
		/// @return true if the current load factor exceeds the maximum load factor; otherwise, false.
		bool ShouldRehash() const noexcept {
			size_type size = m_size.load();
			size_type buckets = m_buckets.size();
			return buckets > 0 && (static_cast<float>(size) / buckets) > m_max_load_factor;
		}

		/// @brief Resizes the bucket array to the specified number of buckets and redistributes all elements accordingly.
		/// @param new_bucket_count The desired number of buckets after rehashing.
		void Rehash(size_type new_bucket_count) {

			if (new_bucket_count <= m_buckets.size()) {
				return;
			}

			auto alloc = m_buckets.get_allocator();

			Buckets new_buckets(alloc);
			new_buckets.reserve(new_bucket_count);
			for (size_type i = 0; i < new_bucket_count; ++i) {
				new_buckets.emplace_back(alloc);
			}

			for (auto& old_bucket : m_buckets) {
				auto lock = old_bucket.mutex.Lock();
				for (auto& key : old_bucket.keys) {
					size_type new_idx = m_hasher(key) % new_bucket_count;
					auto& new_bucket = new_buckets[new_idx];

					auto new_lock = new_bucket.mutex.Lock();
					new_bucket.keys.emplace_back(std::move(key));
				}
			}
			m_buckets = std::move(new_buckets);

		}

		/// @brief Attempts to rehash the container if certain conditions are met.
		void TryRehash() {
			if (ShouldRehash()) {
				Rehash(m_buckets.size() * 2);
			}
		}

		template <std::convertible_to<value_type> CompatibleKey>
		std::pair<BucketModificationPointer, bool> EmplaceImpl(CompatibleKey&& key) {

			auto global_lock = m_buckets_mutex.Lock();

			if (m_buckets.size() == 0) {
				Rehash(16u);
			}
			else {
				TryRehash();
			}

			size_type idx = BucketIndex(key);
			Bucket& bucket = m_buckets[idx];
			auto bucket_lock = bucket.mutex.Lock();

			for (auto& key_ : bucket.keys) {
				if (m_key_equal(key, key_)) {
					return {
						std::piecewise_construct_t(),
						std::forward_as_tuple(&key_, std::move(bucket_lock)),
						std::forward_as_tuple(false)
					};
				}
			}

			auto& emplaced = bucket.keys.emplace_back(std::forward<CompatibleKey>(key));

			m_size.fetch_add(1u, std::memory_order::release);

			return {
				std::piecewise_construct_t(),
				std::forward_as_tuple(&emplaced, std::move(bucket_lock)),
				std::forward_as_tuple(true)
			};

		}

		template <class K, class... Args>
		std::pair<BucketModificationPointer, bool> EmplaceImpl(Args&&... args) {

			auto global_lock = m_buckets_mutex.Lock();

			if (m_buckets.size() == 0) {
				Rehash(16u);
			}
			else {
				TryRehash();
			}

			Key key(std::forward<Args>(args)...);

			size_type idx = BucketIndex(key);
			Bucket& bucket = m_buckets[idx];

			auto bucket_lock = bucket.mutex.Lock();

			for (auto& key_ : bucket.keys) {
				if (m_key_equal(key, key_)) {
					return {
						std::piecewise_construct_t(),
						std::forward_as_tuple(&key_, std::move(bucket_lock)),
						std::forward_as_tuple(false)
					};
				}
			}

			auto& emplaced = bucket.keys.emplace_back(std::move(key));

			m_size.fetch_add(1u, std::memory_order::release);

			return {
				std::piecewise_construct_t(),
				std::forward_as_tuple(&emplaced, std::move(bucket_lock)),
				std::forward_as_tuple(true)
			};

		}


	public:
		/// @brief Constructs a ConcurrentHashSet with a specified number of buckets and optional hash, key equality, and allocator objects.
		/// @param bucket_count The initial number of buckets in the hash map.
		/// @param hash A hash function object to use for hashing keys. Defaults to a default-constructed Hash object.
		/// @param equal A key equality comparison function object. Defaults to a default-constructed KeyEqual object.
		/// @param alloc An allocator object used to allocate memory for the hash map. Defaults to a default-constructed Allocator object.
		explicit ConcurrentHashSet(
			size_type bucket_count,
			Hash const& hash = Hash(),
			KeyEqual const& equal = KeyEqual(),
			Allocator const& alloc = Allocator()
		) : m_buckets(alloc),
			m_buckets_mutex(),
			m_size(0),
			m_max_load_factor(0.75),
			m_hasher(hash),
			m_key_equal(equal) {

			/*
			* 		Buckets m_buckets;
					Mutex m_buckets_mutex;
					std::atomic<size_type> m_size;
					float m_max_load_factor;
					hasher m_hasher;
					key_equal m_key_equal;
			*/

			if (bucket_count > 0) {
				Rehash(bucket_count);
			}
		}

		/// @brief Constructs a ConcurrentHashSet with default settings.
		ConcurrentHashSet()
			: ConcurrentHashSet(0) {
		}

		/// @brief Constructs a ConcurrentHashSet with a specified number of buckets and allocator.
		/// @param bucket_count The initial number of buckets in the hash map.
		/// @param alloc The allocator to use for memory management.
		ConcurrentHashSet(
			size_type bucket_count,
			Allocator const& alloc
		) : ConcurrentHashSet(bucket_count, Hash(), key_equal(), alloc) {

		}

		/// @brief Constructs a ConcurrentHashSet with the specified number of buckets, hash function, and allocator.
		/// @param bucket_count The initial number of buckets in the hash map.
		/// @param hash The hash function to use for hashing keys.
		/// @param alloc The allocator to use for memory management.
		ConcurrentHashSet(
			size_type bucket_count,
			Hash const& hash,
			Allocator const& alloc
		) : ConcurrentHashSet(bucket_count, hash, key_equal(), alloc) {
		}

		ConcurrentHashSet(ConcurrentHashSet const& other)
			: m_buckets_mutex(),
			m_max_load_factor(other.m_max_load_factor),
			m_hasher(other.m_hasher),
			m_key_equal(other.m_key_equal) {

			auto other_lock = other.m_buckets_mutex.LockShared();
			m_buckets = other.m_buckets;
			m_size.store(other.m_size.load(std::memory_order::acquire), std::memory_order::relaxed);

		}

		ConcurrentHashSet(ConcurrentHashSet&& other) noexcept
			: m_buckets_mutex(),
			m_max_load_factor(other.m_max_load_factor),
			m_hasher(std::move(other.m_hasher)),
			m_key_equal(std::move(other.m_key_equal)) {

			auto other_lock = other.m_buckets_mutex.LockShared();
			m_buckets = std::move(other.m_buckets);
			m_size.store(other.m_size.exchange(0u, std::memory_order::acq_rel), std::memory_order::relaxed);

		}

		/// @brief 
		~ConcurrentHashSet() noexcept = default;

		ConcurrentHashSet& operator=(ConcurrentHashSet const& other) {

			if (this != other) {

				auto this_lock = m_buckets_mutex.Lock();
				for (auto& bucket : m_buckets) {
					bucket.Clear();
				}

				m_size.store(0, std::memory_order::release);

				ConcurrentHashSet::ConcurrentHashSet(std::forward<ConcurrentHashSet>(other));

			}

			return *this;

		}

		ConcurrentHashSet& operator=(ConcurrentHashSet&& other) noexcept {

			if (this != other) {

				auto this_lock = m_buckets_mutex.Lock();
				for (auto& bucket : m_buckets) {
					bucket.Clear();
				}

				m_size.store(0, std::memory_order::release);

				ConcurrentHashSet::ConcurrentHashSet(std::forward<ConcurrentHashSet>(other));

			}

			return *this;

		}

		allocator_type get_allocator() const noexcept {
			return m_buckets.get_allocator();
		}

		void max_load_factor(float ml) {
			if (ml <= 0.0f || ml > 1.0f) {
				throw std::invalid_argument("Invalid load factor");
			}
			m_max_load_factor = ml;
		}

		float max_load_factor() const noexcept {
			return m_max_load_factor;
		}

		size_type bucket_count() const noexcept {
			return m_buckets.size();
		}

		std::pair<BucketModificationPointer, bool> insert(value_type const& value) {
			return EmplaceImpl(value);
		}

		std::pair<BucketModificationPointer, bool> insert(value_type&& value) {
			return EmplaceImpl(value);
		}

		AccessingPointer find(Key const& key) {

			auto global_lock = m_buckets_mutex.LockShared();

			if (m_buckets.size() == 0) {
				return nullptr;
			}

			size_type idx = BucketIndex(key);
			Bucket& bucket = m_buckets[idx];
			auto bucket_lock = bucket.mutex.LockShared();

			for (auto const& key_ : bucket.keys) {
				if (m_key_equal(key_, key)) {
					return { &key_, std::move(bucket_lock) };
				}
			}

			return nullptr;

		}

		bool erase(Key const& key) {

			auto buckets_flag = m_buckets_mutex.Lock();

			if (m_buckets.size() == 0) {
				return false;
			}

			size_type idx = BucketIndex(key);
			Bucket& bucket = m_buckets[idx];
			Keys& keys = bucket.keys;
			auto bucket_lock = bucket.mutex.Lock();

			Keys helper(keys.get_allocator());
			helper.reserve(keys.size());

			for (typename Keys::iterator iter = keys.begin(); iter != keys.end(); ++iter) {
				if (m_key_equal(*iter, key)) {
					auto alloc = keys.get_allocator();
					std::allocator_traits<allocator_type>::destroy(alloc, &(*iter));
					std::allocator_traits<allocator_type>::construct(alloc, &(*iter), std::move(keys.back()));
					keys.pop_back();
					m_size.fetch_sub(1u, std::memory_order::release);
					return true;
				}
			}

			return false;

		}

		size_type size() const noexcept {
			return m_size.load(std::memory_order::acquire);
		}

		bool empty() const noexcept {
			return size() == 0;
		}

		void clear() noexcept {

			auto buckets_flag = m_buckets_mutex.Lock();

			size_type buckets_count = m_buckets.size();

			for (size_type i = 0; i < buckets_count; ++i) {
				m_buckets[i].Clear();
			}

			m_size.store(0u, std::memory_order::release);

		}

		BucketModificationReference operator[](Key const& key) {

			auto global_lock = m_buckets_mutex.Lock();

			if (m_buckets.size() == 0) {
				Rehash(16u);
			}
			else {
				TryRehash();
			}

			size_type idx = BucketIndex(key);

			Bucket& bucket = m_buckets[idx];

			auto bucket_lock = bucket.mutex.Lock();

			for (auto& pair : bucket.keys) {
				if (m_key_equal(pair.first, key)) {
					return { &pair.second, std::move(bucket_lock) };
				}
			}

			auto& emplaced = bucket.keys.emplace_back(key, key_type{});

			m_size.fetch_add(1u, std::memory_order::release);

			return { &emplaced.second, std::move(bucket_lock) };

		}

		/// @brief Returns a reference to the value associated with the specified key, along with a lock for safe modification within a concurrent hash table.
		/// @param key The key whose associated value is to be accessed and potentially modified.
		/// @return A BucketModificationReference containing a pointer to the value associated with the given key and a lock for the corresponding bucket. Throws std::out_of_range if the key does not exist.
		BucketModificationReference at(Key const& key) {

			auto global_lock = m_buckets_mutex.Lock();

			if (m_buckets.size() == 0) {
				Rehash(16u);
			}
			else {
				TryRehash();
			}

			size_type idx = BucketIndex(key);

			Bucket& bucket = m_buckets[idx];

			auto bucket_lock = bucket.mutex.Lock();

			for (auto& pair : bucket.keys) {
				if (m_key_equal(pair.first, key)) {
					return { &pair.second, std::move(bucket_lock) };
				}
			}

			throw std::out_of_range("the key does not exist in the hash table");

		}


		template <class... Args>
		std::pair<BucketModificationPointer, bool> emplace(Args&&... args) {
			return EmplaceImpl(std::forward<Args>(args)...);
		}

		/// @brief Creates and returns a Modifier object associated with the current instance.
		/// @return A Modifier object constructed with the current instance as its argument.
		Modifier LockedModifier() {
			return Modifier(this);
		}

		/// @brief Creates and returns a View object associated with the current instance.
		/// @return A View object constructed from the current instance.
		View LockedView() const {
			return View(this);
		}

		key_type& UnsafeAt(Key const& key) {

			auto global_lock = m_buckets_mutex.Lock();

			if (m_buckets.size() == 0) {
				Rehash(16u);
			}
			else {
				TryRehash();
			}

			size_type idx = BucketIndex(key);

			Bucket& bucket = m_buckets[idx];

			auto bucket_lock = bucket.mutex.Lock();

			for (auto& key_ : bucket.keys) {
				if (m_key_equal(key_, key)) {
					return key_;
				}
			}

			throw std::out_of_range("the key does not exist in the hash table");

		}

	};

}
