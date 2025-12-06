export module static_lock_free_hash_table;
import std;
import concurrent_container_base;

namespace fyuu_engine::concurrency {

	export template <
		class Key,
		class T,
		std::size_t bucket_size = 128u,
		class Hash = std::hash<Key>,
		class KeyEqual = std::equal_to<Key>,
		class Allocator = std::allocator<std::pair<ImmutableKey<Key>, T>>
	>
	class StaticLockFreeHashTable {
	public:
		using key_type = Key;
		using mapped_type = T;
		using value_type = std::pair<ImmutableKey<Key>, T>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using hasher = Hash;
		using key_equal = KeyEqual;
		using allocator_type = Allocator;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = typename std::allocator_traits<Allocator>::pointer;
		using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

		class SafeReference {
		private:
			mapped_type& m_val;
			std::atomic<size_type>& m_external_ref_count;
		public:
			SafeReference(mapped_type& val, std::atomic<size_type>& external_ref_count)
				: m_val(val), m_external_ref_count(external_ref_count) {
				m_external_ref_count.fetch_add(1u, std::memory_order::release);
			}

			SafeReference(SafeReference const& other) noexcept
				: m_val(other.m_val), m_external_ref_count(other.m_external_ref_count) {
				m_external_ref_count.fetch_add(1u, std::memory_order::release);
			}

			~SafeReference() noexcept {
				m_external_ref_count.fetch_sub(1u, std::memory_order::release);
				m_external_ref_count.notify_one();
			}

			mapped_type& Get() noexcept {
				return m_val;
			}

			SafeReference& operator=(SafeReference const& other) {
				m_val = other.m_val;
				return *this;
			}

			SafeReference& operator=(mapped_type const& val) {
				m_val = val;
				return *this;
			}

			SafeReference& operator=(mapped_type&& val) noexcept {
				m_val = std::move(val);
				return *this;
			}

			operator mapped_type() const {
				return m_val;
			}

		};

		class SafeConstReference {
		private:
			mapped_type const& m_val;
			std::atomic<size_type>& m_external_ref_count;

		public:
			SafeConstReference(mapped_type const& val, std::atomic<size_type>& external_ref_count)
				: m_val(val), m_external_ref_count(external_ref_count) {
				m_external_ref_count.fetch_add(1u, std::memory_order::release);
			}

			SafeConstReference(SafeConstReference const& other) noexcept
				: m_val(other.m_val), m_external_ref_count(other.m_external_ref_count) {
				m_external_ref_count.fetch_add(1u, std::memory_order::release);
			}

			~SafeConstReference() noexcept {
				m_external_ref_count.fetch_sub(1u, std::memory_order::release);
				m_external_ref_count.notify_one();
			}

			mapped_type const& Get() const noexcept {
				return m_val;
			}

			operator mapped_type() const {
				return m_val;
			}

		};

	private:
		struct Node {
			value_type value;
			std::atomic<Node*> next = nullptr;
			mutable std::atomic<size_type> external_ref_count = 0u;

			template <typename... Args>
			Node(Args&&... args)
				: value(std::forward<Args>(args)...) {
			}
		};

		using NodeAlloc = typename std::allocator_traits<allocator_type>::template rebind_alloc<Node>;

		hasher m_hash;
		key_equal m_equal;
		NodeAlloc m_alloc;
		std::atomic<size_type> m_size = 0u;
		std::array<std::atomic<Node*>, bucket_size> m_buckets{ nullptr };

		template <class... Args>
		std::expected<Node*, std::string> CreateNode(Args&&... args) {

			using AllocTraits = std::allocator_traits<NodeAlloc>;

			Node* node = nullptr;

			try {
				node = AllocTraits::allocate(m_alloc, 1);
				AllocTraits::construct(m_alloc, node, std::forward<Args>(args)...);
			}
			catch (std::exception const& ex) {
				return std::unexpected(ex.what());
			}

			return node;

		}

		void DestroyNode(Node* node) {

			using AllocTraits = std::allocator_traits<NodeAlloc>;

			AllocTraits::destroy(m_alloc, node);
			AllocTraits::deallocate(m_alloc, node, 1);

		}

		template <std::convertible_to<value_type> Pair>
		std::expected<std::pair<SafeReference, bool>, std::string> EmplaceImpl(Pair&& pair) {

			auto&& [key, value] = pair;

			size_type bucket_index = m_hash(key) % bucket_size;

			std::atomic<Node*>& bucket = m_buckets[bucket_index];
			Node* head = bucket.load(std::memory_order::acquire);

			Node* current = head;
			while (current) {

				/*
				*	check if key exists
				*/

				auto&& [current_key, current_val] = current->value;
				if (m_equal(current_key, key)) {
					return std::make_pair(SafeReference(current_val, current->external_ref_count), false);
				}

				current = current->next.load(std::memory_order::acquire);

			}

			std::expected<Node*, std::string> expected_new_node = CreateNode(std::forward<Pair>(pair));

			if (!expected_new_node) {
				return std::unexpected(std::move(expected_new_node.error()));
			}

			Node* new_node = *expected_new_node;

			/// As new head
			new_node->next.store(head, std::memory_order::relaxed);
			while (!bucket.compare_exchange_weak(head, new_node, std::memory_order::release, std::memory_order::acquire)) {
				/// CAS failed, retry

				new_node->next.store(head, std::memory_order::relaxed);

				Node* current = head;
				while (current) {

					/*
					*	re-check if key exists
					*/

					auto&& [current_key, current_val] = current->value;
					if (m_equal(current_key, key)) {
						DestroyNode(new_node);
						return std::make_pair(SafeReference(current_val, current->external_ref_count), false);
					}

					current = current->next.load(std::memory_order::acquire);

				}

			}

			/*
			*	inserted
			*/

			m_size.fetch_add(1u, std::memory_order::release);
			return std::make_pair(SafeReference(new_node->value.second, new_node->external_ref_count), true);

		}


		template <class K, class... Args>
		std::expected<std::pair<SafeReference, bool>, std::string> EmplaceImpl(K&& key, Args&&... args) {

			size_type bucket_index = m_hash(key) % bucket_size;

			std::atomic<Node*>& bucket = m_buckets[bucket_index];
			Node* head = bucket.load(std::memory_order::acquire);

			Node* current = head;
			while (current) {

				/*
				*	check if key exists
				*/

				auto&& [current_key, current_val] = current->value;
				if (m_equal(current_key, key)) {
					return std::make_pair(SafeReference(current_val, current->external_ref_count), false);
				}

				current = current->next.load(std::memory_order::acquire);

			}

			std::expected<Node*, std::string> expected_new_node = CreateNode(std::forward<K>(key), std::forward<Args>(args)...);

			if (!expected_new_node) {
				return std::unexpected(std::move(expected_new_node.error()));
			}

			Node* new_node = *expected_new_node;

			/// As new head
			new_node->next.store(head, std::memory_order::relaxed);
			while (!bucket.compare_exchange_weak(head, new_node, std::memory_order::release, std::memory_order::acquire)) {
				/// CAS failed, retry

				new_node->next.store(head, std::memory_order::relaxed);

				Node* current = head;
				while (current) {

					/*
					*	re-check if key exists
					*/

					auto&& [current_key, current_val] = current->value;
					if (m_equal(current_key, key)) {
						DestroyNode(new_node);
						return std::make_pair(SafeReference(current_val, current->external_ref_count), false);
					}

					current = current->next.load(std::memory_order::acquire);

				}

			}

			/*
			*	inserted
			*/

			m_size.fetch_add(1u, std::memory_order::release);
			return std::make_pair(SafeReference(new_node->value.second, new_node->external_ref_count), true);

		}

	public:
		StaticLockFreeHashTable(
			Hash const& hash = Hash(),
			key_equal const& equal = key_equal(),
			Allocator const& alloc = Allocator()
		) : m_hash(hash),
			m_equal(equal),
			m_alloc(alloc) {

		}

		StaticLockFreeHashTable(StaticLockFreeHashTable&& other) noexcept
			: m_hash(std::move(other.m_hash)),
			m_equal(std::move(other.m_equal)),
			m_alloc(std::move(other.m_alloc)),
			m_size(other.m_size.exchange(0u, std::memory_order::acq_rel)) {
			for (size_type i = 0; i < bucket_size; ++i) {
				m_buckets[i].store(
					other.m_buckets[i].exchange(
						nullptr,
						std::memory_order::acq_rel
					),
					std::memory_order::relaxed
				);
			}
		}

		size_type size() const noexcept {
			return m_size.load(std::memory_order::acquire);
		}

		auto insert(value_type const& value) {
			return EmplaceImpl(value);
		}

		auto insert(value_type&& value) {
			return EmplaceImpl(value);
		}

		template <class... Args>
		auto emplace(Args&&... args) {
			return EmplaceImpl(std::forward<Args>(args)...);
		}

		std::expected<SafeReference, std::string_view> find(Key const& key) {

			size_type bucket_index = m_hash(key) % bucket_size;

			std::atomic<Node*>& bucket = m_buckets[bucket_index];
			Node* head = bucket.load(std::memory_order::acquire);

			Node* current = head;
			while (current) {

				/*
				*	check if key exists
				*/

				auto&& [current_key, current_val] = current->value;
				if (m_equal(current_key, key)) {
					return SafeReference(current_val, current->external_ref_count);
				}

				current = current->next.load(std::memory_order::acquire);

			}

			return std::unexpected("No key found");

		}

		std::expected<SafeConstReference, std::string_view> find(Key const& key) const {

			size_type bucket_index = m_hash(key) % bucket_size;

			std::atomic<Node*>& bucket = m_buckets[bucket_index];
			Node* head = bucket.load(std::memory_order::acquire);

			Node* current = head;
			while (current) {

				/*
				*	check if key exists
				*/

				auto&& [current_key, current_val] = current->value;
				if (m_equal(current_key, key)) {
					return SafeConstReference(current_val, current->external_ref_count);
				}

				current = current->next.load(std::memory_order::acquire);

			}

			return std::unexpected("No key found");
		}

		bool erase(Key const& key) {

			size_type bucket_index = m_hash(key) % bucket_size;
			std::atomic<Node*>& bucket = m_buckets[bucket_index];

			Node* head = bucket.load(std::memory_order::acquire);
			Node* prev = nullptr;
			Node* current = head;

			while (current) {

				/*
				*	search the key to remove
				*/

				auto&& [current_key, current_value] = current->value;

				if (m_equal(current_key, key)) {
					break;
				}
				prev = current;
				current = current->next.load(std::memory_order_acquire);
			}

			if (!current) {
				// not found
				return false;
			}

			if (prev) {
				/*
				*	remove middle or tail node
				*/
				Node* next = current->next.load(std::memory_order::acquire);
				prev->next.store(next, std::memory_order::release);
			}
			else {
				/*
				*	remove head
				*/
				Node* next = current->next.load(std::memory_order::acquire);
				while (!bucket.compare_exchange_weak(head, next, std::memory_order::release, std::memory_order::acquire)) {
					/*
					*  CAS failed
					*/
					if (head != current) {
						// list modified, restart
						return erase(key);
					}
					next = current->next.load(std::memory_order::acquire);
				}
			}

			size_type external_ref_count;
			do {
				external_ref_count = current->external_ref_count.load(std::memory_order::acquire);
				if (external_ref_count > 0) {
					current->external_ref_count.wait(external_ref_count, std::memory_order::relaxed);
				}
			} while (external_ref_count > 0);

			DestroyNode(current);
			m_size.fetch_sub(1, std::memory_order::relaxed);

			return true;

		}

		void clear() {
			for (size_type i = 0; i < bucket_size; ++i) {
				Node* head = m_buckets[i].exchange(nullptr, std::memory_order::acq_rel);
				while (head) {
					Node* next = head->next.load(std::memory_order::acquire);

					size_type external_ref_count;
					do {
						external_ref_count = head->external_ref_count.load(std::memory_order::acquire);
						if (external_ref_count > 0) {
							head->external_ref_count.wait(external_ref_count, std::memory_order::relaxed);
						}
					} while (external_ref_count > 0);

					DestroyNode(head);

					m_size.fetch_sub(1u, std::memory_order::release);
					head = next;
				}
			}
		}

	};

}