module;
#include <cassert>
export module lock_free_hash_table;
import std;
import concurrent_container_base;

namespace fyuu_engine::concurrency {

	export template <
		class Key,
		class T,
		class Hash = std::hash<Key>,
		class KeyEqual = std::equal_to<Key>,
		class Allocator = std::allocator<std::pair<ImmutableKey<Key>, T>>
	>
	class LockFreeHashTable {
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

	private:
		struct Bucket {

			struct alignas(std::hardware_destructive_interference_size) Slot {

				mutable std::atomic<size_type> ref_count = 0u;
				std::optional<value_type> value;

				void WaitForExternalReference() const noexcept {

					size_type current_ref_count;
					do {
						current_ref_count = ref_count.load(std::memory_order::relaxed);
						if (current_ref_count > 0) {
							ref_count.wait(current_ref_count, std::memory_order::relaxed);
						}
					} while (current_ref_count > 0);

				}

				void AddReference() const noexcept {
					ref_count.fetch_add(1u, std::memory_order::relaxed);
				}

				void ReleaseReference() const noexcept {
					size_type old = ref_count.fetch_sub(1u, std::memory_order::relaxed);
					if (old == 1) {
						ref_count.notify_one();
					}
				}

				class SafeReference {
				private:
					Slot* m_node;

				public:
					SafeReference(Slot* node) noexcept
						: m_node(node) {
						if (m_node) {
							m_node->AddReference();
						}
					}

					SafeReference(SafeReference const& other) noexcept
						: m_node(other.m_node) {
						if (m_node) {
							m_node->AddReference();
						}
					}

					SafeReference(SafeReference&& other) noexcept
						: m_node(std::exchange(other.m_node, nullptr)) {

					}

					~SafeReference() noexcept {
						if (m_node) {
							m_node->ReleaseReference();
						}
					}

					mapped_type& Get() noexcept {
						return m_node->value->second;
					}

					SafeReference& operator=(SafeReference const& other) noexcept {
						if (this != &other) {
							if (m_node) {
								m_node->ReleaseReference();
							}
							m_node = other.m_node;
							if (m_node) {
								m_node->AddReference();
							}
						}
						return *this;
					}

					SafeReference& operator=(mapped_type const& val) {
						m_node->value->second = val;
						return *this;
					}

					SafeReference& operator=(mapped_type&& val) noexcept {
						m_node->value->second = std::move(val);
						return *this;
					}

					operator mapped_type() const {
						return m_node->value.second;
					}

					operator bool() const noexcept {
						return m_node;
					}

					bool operator==(mapped_type const& val) const noexcept {
						return m_node->value->second == val;
					}

				};

				class SafeConstReference {
				private:
					Slot const* m_node;

				public:
					SafeConstReference(Slot const* node) noexcept
						: m_node(node) {
						if (m_node) {
							m_node->AddReference();
						}
					}

					SafeConstReference(SafeConstReference const& other) noexcept
						: m_node(other.m_node) {
						if (m_node) {
							m_node->AddReference();
						}
					}

					SafeConstReference(SafeConstReference&& other) noexcept
						: m_node(std::exchange(other.m_node, nullptr)) {

					}

					~SafeConstReference() noexcept {
						if (m_node) {
							m_node->ReleaseReference();
						}
					}

					mapped_type const& Get() const noexcept {
						return m_node->value.second;
					}

					SafeConstReference& operator=(SafeConstReference const& other) noexcept {
						if (this != &other) {
							if (m_node) {
								m_node->ReleaseReference();
							}
							m_node = other.m_node;
							if (m_node) {
								m_node->AddReference();
							}
						}
						return *this;
					}

					operator mapped_type() const {
						return m_node->value->second;
					}

					operator bool() const noexcept {
						return m_node;
					}

					bool operator==(mapped_type const& val) const noexcept {
						return m_node->value->second == val;
					}

				};

			};

			static constexpr size_type SlotCount = std::hardware_destructive_interference_size;

			std::array<Slot, SlotCount> slots;
			alignas(std::hardware_destructive_interference_size) std::atomic<size_type> free_slots_bitmap{ std::numeric_limits<size_type>::max()};
			std::atomic<Bucket*> next;

			std::optional<size_type> AcquireFreeSlotIndex() noexcept {
				size_type bitmap = free_slots_bitmap.load(std::memory_order::relaxed);
				while (bitmap != 0) {
					size_type idx = std::countr_zero(bitmap);
					size_type mask = size_type(1) << idx;

					size_type expected = bitmap;
					size_type desired = bitmap & ~mask;
					if (free_slots_bitmap.compare_exchange_weak(
						expected, desired,
						std::memory_order::release,
						std::memory_order::relaxed)) {
						return idx;
					}
					bitmap = expected;
				}
				return std::nullopt;
			}

			void MarkSlotFree(size_type idx) noexcept {
				size_type mask = size_type(1) << idx;
				free_slots_bitmap.fetch_or(mask, std::memory_order::release);
			}


		};

		using Slot = Bucket::Slot;
		using BucketAlloc = typename std::allocator_traits<allocator_type>::template rebind_alloc<Bucket>;

	public:
		using SafeReference = Slot::SafeReference;
		using SafeConstReference = Slot::SafeConstReference;

	private:
		struct Table {
			std::vector<Bucket, BucketAlloc> buckets;
			std::atomic<size_type> size;
			std::atomic_flag read_only;
			hasher hash;
			key_equal equal;
			mutable std::atomic<size_type> ref_count;

			using SafeReference = Bucket::Slot::SafeReference;
			using SafeConstReference = Bucket::Slot::SafeConstReference;

			Table(size_type capacity, BucketAlloc const& alloc, hasher const& hash_, key_equal const& equal_)
				: buckets(capacity, alloc), hash(hash_), equal(equal_) {

			}

			~Table() noexcept {

				WaitForExternalReference();

				BucketAlloc alloc = buckets.get_allocator();
				using BucketAllocTraits = std::allocator_traits<BucketAlloc>;

				for (Bucket& bucket : buckets) {

					Bucket* current = bucket.next.load(std::memory_order::relaxed);

					while (current) {
						Bucket* next = current->next.load(std::memory_order::relaxed);
						BucketAllocTraits::destroy(alloc, current);
						BucketAllocTraits::deallocate(alloc, current, 1);
						current = next;
					}

				}

				buckets.clear();

			}

			size_type Capacity() const noexcept {
				return buckets.size();
			}

			void WaitForExternalReference() const noexcept {

				size_type current_ref_count;
				do {
					current_ref_count = ref_count.load(std::memory_order::acquire);
					if (current_ref_count > 0) {
						ref_count.wait(current_ref_count, std::memory_order::relaxed);
					}
				} while (current_ref_count > 0);

			}

			void AddReference() const noexcept {
				ref_count.fetch_add(1u, std::memory_order::relaxed);
			}

			void ReleaseReference() const noexcept {
				size_type old = ref_count.fetch_sub(1u, std::memory_order::relaxed);
				if (old == 1) {
					ref_count.notify_one();
				}
			}

			SafeReference Find(key_type const& key) noexcept {

				size_type capacity = Capacity();
				if (capacity == 0) {
					return nullptr;
				}

				size_type index = hash(key) % capacity;

				for (Bucket* current = &buckets[index]; current; current = current->next.load(std::memory_order::relaxed)) {
					for (Slot& slot : current->slots) {

						if (!slot.value) {
							continue;
						}

						auto&& [slot_key, slot_value] = *(slot.value);

						if (equal(key, slot_key)) {
							return &slot;
						}
					}
				}

				return nullptr;

			}

			SafeConstReference Find(key_type const& key) const noexcept {

				size_type capacity = Capacity();
				if (capacity == 0) {
					return nullptr;
				}

				size_type index = hash(key) % capacity;

				for (Bucket const* current = &buckets[index]; current; current = current->next.load(std::memory_order::relaxed)) {
					for (Slot const& slot : current->slots) {

						if (!slot.value) {
							continue;
						}

						auto&& [slot_key, slot_value] = *(slot.value);

						if (equal(key, slot_key)) {
							return &slot;
						}
					}
				}

				return nullptr;

			}

			template <std::convertible_to<value_type> Pair>
			SafeReference EmplaceSlotPair(Bucket* current, Pair&& pair) {

				std::optional<size_type> slot_index = current->AcquireFreeSlotIndex();
				if (!slot_index) {
					return nullptr;
				}

				Slot& slot = current->slots[*slot_index];
				try {
					slot.value.emplace(std::forward<Pair>(pair));
				}
				catch (...) {
					current->MarkSlotFree(*slot_index);
					throw;
				}

				size.fetch_add(1u, std::memory_order::relaxed);
				return &slot;

			}

			template <std::convertible_to<value_type> Pair>
			std::expected<std::pair<SafeReference, bool>, std::string> Emplace(Pair&& pair) {

				if (read_only.test(std::memory_order::relaxed)) {
					return std::unexpected("read only");
				}

				auto&& [key, value] = pair;

				if (SafeReference ref = Find(key)) {
					/*
					*	emplacement failed if the key exists
					*/
					return std::make_pair(ref, false);
				}

				size_type index = hash(key) % buckets.size();

				Bucket& bucket = buckets[index];

				for (Bucket* current = &bucket; current; current = current->next.load(std::memory_order::relaxed)) {
					try {
						if (SafeReference ref = EmplaceSlotPair(current, std::forward<Pair>(pair))) {
							return std::make_pair(ref, true);
						}
					}
					catch (std::exception const& ex) {
						return std::unexpected(ex.what());
					}
				}

				/*
				*	all buckets are full, try to create a new bucket
				*/

				BucketAlloc bucket_alloc = buckets.get_allocator();
				using BucketAllocTraits = std::allocator_traits<BucketAlloc>;
				Bucket* new_bucket = nullptr;
				SafeReference ref = nullptr;

				while (!ref) {
					if (!new_bucket) {
						try {
							new_bucket = BucketAllocTraits::allocate(bucket_alloc, 1);
							BucketAllocTraits::construct(bucket_alloc, new_bucket);
						}
						catch (std::exception const& ex) {
							if (new_bucket) {
								BucketAllocTraits::destroy(bucket_alloc, new_bucket);
								BucketAllocTraits::deallocate(bucket_alloc, new_bucket, 1);
							}
							return std::unexpected(ex.what());
						}
						Bucket* head = bucket.next.load(std::memory_order::acquire);
						new_bucket->next.store(head, std::memory_order::relaxed);
						if (!bucket.next.compare_exchange_weak(
							head,
							new_bucket,
							std::memory_order::release,
							std::memory_order::acquire)) {
							// another thread added a bucket, discard ours
							BucketAllocTraits::destroy(bucket_alloc, new_bucket);
							BucketAllocTraits::deallocate(bucket_alloc, new_bucket, 1);
						}
					}

					try {
						ref = EmplaceSlotPair(new_bucket, std::forward<Pair>(pair));
						if (ref) {
							break;
						}
						new_bucket = nullptr;
					}
					catch (std::exception const& ex) {
						return std::unexpected(ex.what());
					}

				}

				return std::make_pair(ref, true);

			}

			bool Erase(key_type const& key) {

				size_type index = hash(key) % buckets.size();

				for (Bucket* current = &buckets[index]; current; current = current->next.load(std::memory_order::relaxed)) {
					for (Slot& slot : current->slots) {

						if (!slot.value) {
							continue;
						}

						auto&& [slot_key, slot_value] = *(slot.value);

						if (!equal(key, slot_key)) {
							continue;
						}

						slot.WaitForExternalReference();
						slot.value.reset();

						std::uintptr_t slot_addr = reinterpret_cast<std::uintptr_t>(&slot);
						std::uintptr_t slots_array_addr = reinterpret_cast<std::uintptr_t>(current);

						std::ptrdiff_t offset = slot_addr - slots_array_addr;
						size_type slot_index = offset / sizeof(Slot);

						current->MarkSlotFree(slot_index);
						size.fetch_sub(1u, std::memory_order::relaxed);

						return true;

					}
				}

				return false;

			}

		};

		struct TablePtr {
		private:
			Table* m_table = nullptr;

		public:
			TablePtr() = default;

			TablePtr(Table* table) noexcept
				: m_table(table) {
				if (m_table) {
					m_table->AddReference();
				}
			}

			TablePtr(TablePtr const& other) noexcept
				: m_table(other.m_table) {
				if (m_table) {
					m_table->AddReference();
				}
			}

			TablePtr(TablePtr&& other) noexcept
				: m_table(std::exchange(other.m_table, nullptr)) {

			}

			~TablePtr() noexcept {
				if (m_table) {
					m_table->ReleaseReference();
				}
			}

			TablePtr& operator=(TablePtr const& other) noexcept {
				if (this != &other) {
					if (m_table) {
						m_table->ReleaseReference();
					}
					m_table = other.m_table;
					if (m_table) {
						m_table->AddReference();
					}
				}
				return *this;
			}

			Table* Detach() noexcept {
				if (m_table) {
					m_table->ReleaseReference();
				}
				return std::exchange(m_table, nullptr);
			}

			operator bool() const noexcept {
				return m_table;
			}

			operator Table*() const noexcept {
				return m_table;
			}

			Table* operator->() const noexcept {
				return m_table;
			}

		};

		struct ConstTablePtr {
		private:
			Table const* m_table = nullptr;

		public:
			ConstTablePtr() = default;

			ConstTablePtr(Table* table) noexcept
				: m_table(table) {
				if (m_table) {
					m_table->AddReference();
				}
			}

			ConstTablePtr(ConstTablePtr const& other) noexcept
				: m_table(other.m_table) {
				if (m_table) {
					m_table->AddReference();
				}
			}

			ConstTablePtr(ConstTablePtr&& other) noexcept
				: m_table(std::exchange(other.m_table, nullptr)) {

			}

			~ConstTablePtr() noexcept {
				if (m_table) {
					m_table->ReleaseReference();
				}
			}

			ConstTablePtr& operator=(ConstTablePtr const& other) noexcept {
				if (this != &other) {
					if (m_table) {
						m_table->ReleaseReference();
					}
					m_table = other.m_table;
					if (m_table) {
						m_table->AddReference();
					}
				}
				return *this;
			}

			operator bool() const noexcept {
				return m_table;
			}

			Table const* operator->() const noexcept {
				return m_table;
			}

		};

		using TableAlloc = typename std::allocator_traits<allocator_type>::template rebind_alloc<Table>;

		enum class CurrentTable : std::uint8_t {
			Invalid,
			TableA,
			TableB,
		};

		std::atomic_flag m_migrating;
		hasher m_hash;
		key_equal m_equal;
		TableAlloc m_table_alloc;
		std::atomic_uint8_t m_active_table;
		float m_max_load_factor = 0.75f;
		std::atomic<Table*> m_table_a;
		std::atomic<Table*> m_table_b;
		std::jthread m_migration_thread;

		TablePtr CreateTable(size_type capacity) {

			using AllocTraits = std::allocator_traits<TableAlloc>;

			Table* table = nullptr;

			table = AllocTraits::allocate(m_table_alloc, 1);
			AllocTraits::construct(m_table_alloc, table, capacity, m_table_alloc, m_hash, m_equal);

			return table;

		}

		void DestroyTable(TablePtr& table) noexcept {

			if (!table) {
				return;
			}

			using TableTraits = std::allocator_traits<TableAlloc>;

			try {
				Table* detached = table.Detach();
				TableTraits::destroy(m_table_alloc, detached);
				TableTraits::deallocate(m_table_alloc, detached, 1);
			}
			catch (...) {

			}

		}

		TablePtr ActiveTable() noexcept {
			auto active_table = static_cast<CurrentTable>(m_active_table.load(std::memory_order::relaxed));
			switch (active_table) {
			case CurrentTable::TableA:
				return m_table_a.load(std::memory_order::relaxed);
			case CurrentTable::TableB:
				return m_table_b.load(std::memory_order::relaxed);
			default:
				return nullptr;
			}
		}

		ConstTablePtr ActiveTable() const noexcept {
			auto active_table = static_cast<CurrentTable>(m_active_table.load(std::memory_order::relaxed));
			switch (active_table) {
			case CurrentTable::TableA:
				return m_table_a.load(std::memory_order::relaxed);
			case CurrentTable::TableB:
				return m_table_b.load(std::memory_order::relaxed);
			default:
				return nullptr;
			}
		}

		TablePtr InactiveTable() noexcept {
			auto active_table = static_cast<CurrentTable>(m_active_table.load(std::memory_order::relaxed));
			switch (active_table) {
			case CurrentTable::TableA:
				return m_table_b.load(std::memory_order::relaxed);
			case CurrentTable::TableB:
				return m_table_a.load(std::memory_order::relaxed);
			default:
				return nullptr;
			}
		}

		ConstTablePtr InactiveTable() const noexcept {
			auto active_table = static_cast<CurrentTable>(m_active_table.load(std::memory_order::relaxed));
			switch (active_table) {
			case CurrentTable::TableA:
				return m_table_b.load(std::memory_order::relaxed);
			case CurrentTable::TableB:
				return m_table_a.load(std::memory_order::relaxed);
			default:
				return nullptr;
			}
		}


		bool NeedRehash() const noexcept {

			ConstTablePtr table = ActiveTable();
			if (!table) {
				return true;
			}

			float load_factor = static_cast<float>(table->size.load(std::memory_order::relaxed))
				/ static_cast<float>(table->Capacity());

			return load_factor > m_max_load_factor;

		}

		void MigrateData(std::stop_token token, TablePtr& old_table, TablePtr& new_table) {

			old_table->read_only.test_and_set(std::memory_order::relaxed);

			while (!token.stop_requested() && old_table->size.load(std::memory_order::relaxed) > 0u) {
				for (Bucket& bucket : old_table->buckets) {
					for (Bucket* current = &bucket; current; current = current->next.load(std::memory_order::relaxed)) {
						for (Slot& slot : current->slots) {

							if (!slot.value) {
								continue;
							}

							auto&& [slot_key, slot_value] = *(slot.value);

							slot.WaitForExternalReference();
							new_table->Emplace(std::move(*(slot.value)));

							old_table->size.fetch_sub(1u, std::memory_order::relaxed);

						}
					}
				}
			}

		}

		void MigrateDataWorker(std::stop_token token, TablePtr& old_table, TablePtr& new_table) {
			MigrateData(token, old_table, new_table);
			DestroyTable(old_table);
		}

		void Rehash() {

			if (m_migrating.test_and_set(std::memory_order::acq_rel)) {
				return;
			}

			TablePtr old_table = ActiveTable();
			size_type new_capacity = std::max(size_type(16u), old_table->Capacity() * 2);
			using TableAllocTraits = std::allocator_traits<TableAlloc>;
			TablePtr new_table = CreateTable(new_capacity);
			std::atomic<Table*>* inactive_table;

			auto active_table = static_cast<CurrentTable>(m_active_table.load(std::memory_order::relaxed));
			switch (active_table) {
			case CurrentTable::TableA:
				inactive_table = &m_table_b;
				break;

			case CurrentTable::TableB:
				inactive_table = &m_table_a;
				break;
			}

			inactive_table->store(new_table, std::memory_order::relaxed);
			m_active_table.store(
				static_cast<std::uint8_t>(
					m_active_table.load(std::memory_order::relaxed) ==
					static_cast<std::uint8_t>(CurrentTable::TableA) ?
					CurrentTable::TableB : CurrentTable::TableA
					),
				std::memory_order::relaxed
			);

			m_migration_thread = std::jthread(
				[this, old_table, new_table, inactive_table](std::stop_token token) mutable {
					try {
						MigrateDataWorker(token, old_table, new_table);
					}
					catch (...) {
					}
					m_migrating.clear(std::memory_order::relaxed);
				}
			);

		}

		template <std::convertible_to<value_type> Pair>
		std::expected<std::pair<SafeReference, bool>, std::string> EmplaceImpl(Pair&& pair) {

			try {
				if (NeedRehash()) {
					Rehash();
				}
			}
			catch (std::exception const& ex) {
				std::unexpected(ex.what());
			}

			TablePtr active_table;
			bool read_only = false;

			do {
				active_table = ActiveTable();
				if (!active_table) {
					read_only = true;
					continue;
				}
				read_only = active_table->read_only.test(std::memory_order::relaxed);
			} while (read_only);

			return active_table->Emplace(std::forward<Pair>(pair));

		}

	public:
		LockFreeHashTable(
			Hash const& hash = Hash(),
			key_equal const& equal = key_equal(),
			Allocator const& alloc = Allocator()
		) : m_hash(hash),
			m_equal(equal),
			m_table_alloc(alloc),
			m_active_table(static_cast<std::uint8_t>(CurrentTable::TableA)),
			m_table_a(CreateTable(16u)) {

		}

		LockFreeHashTable(LockFreeHashTable&& other) noexcept
			: m_hash(std::move(other.m_hash)),
			m_equal(std::move(other.m_equal)),
			m_table_alloc(std::move(other.m_table_alloc)),
			m_active_table(other.m_active_table.exchange(static_cast<std::uint8_t>(CurrentTable::Invalid), std::memory_order::acq_rel)),
			m_max_load_factor(std::exchange(other.m_max_load_factor, 0.0f)),
			m_table_a(other.m_table_a.exchange(nullptr, std::memory_order::acq_rel)),
			m_table_b(other.m_table_b.exchange(nullptr, std::memory_order::acq_rel)) {

		}

		~LockFreeHashTable() noexcept {

			if (m_migration_thread.joinable()) {
				m_migration_thread.request_stop();
				m_migration_thread.join();
			}

			TablePtr active_table = ActiveTable();

			DestroyTable(active_table);

			m_table_a.store(nullptr, std::memory_order::relaxed);
			m_table_b.store(nullptr, std::memory_order::relaxed);

		}

		size_type size() const noexcept {
			Table const* table_a = m_table_a.load(std::memory_order::relaxed);
			Table const* table_b = m_table_b.load(std::memory_order::relaxed);
			size_type table_a_size = table_a ? table_a->size.load(std::memory_order::relaxed) : 0u;
			size_type table_b_size = table_b ? table_b->size.load(std::memory_order::relaxed) : 0u;
			return table_a_size + table_b_size;
		}

		bool empty() const noexcept {
			return size() == size_type(0);
		}

		auto insert(value_type const& value) {
			return EmplaceImpl(value);
		}

		auto insert(value_type&& value) {
			return EmplaceImpl(value);
		}

		template <class... Args>
		auto emplace(Args&&... args) {
			return EmplaceImpl(value_type(std::forward<Args>(args)...));
		}

		SafeReference find(Key const& key) {

			TablePtr active_table = ActiveTable();
			if (active_table) {
				if (auto result = active_table->Find(key)) {
					return result;
				}
			}

			if (m_migrating.test(std::memory_order::relaxed)) {
				TablePtr inactive_table = InactiveTable();
				if (auto result = inactive_table ? inactive_table->Find(key) : nullptr) {
					return result;
				}
			}

			return nullptr;

		}

		SafeConstReference find(Key const& key) const {

			TablePtr active_table = ActiveTable();
			if (active_table) {
				if (auto result = active_table->Find(key)) {
					return result;
				}
			}

			if (m_migrating.test(std::memory_order::relaxed)) {
				TablePtr inactive_table = InactiveTable();
				if (auto result = inactive_table ? inactive_table->Find(key) : nullptr) {
					return result;
				}
			}

			return nullptr;

		}

		SafeReference operator[](Key const& key) {
			return find(key);
		}

		SafeConstReference operator[](Key const& key) const {
			return find(key);
		}

		bool erase(Key const& key) {
			TablePtr active_table = ActiveTable();
			return active_table->Erase(key);
		}

		void clear() {
			TablePtr active_table = ActiveTable();
			DestroyTable(active_table);
			m_table_a.store(CreateTable(16u), std::memory_order::release);
			m_active_table.store(static_cast<std::uint8_t>(CurrentTable::TableA), std::memory_order::release);
		}

		float max_load_factor() const noexcept {
			return m_max_load_factor;
		}

		void max_load_factor(float ml) {
			m_max_load_factor = ml;
		}

	};

	namespace test::lock_free_hash_table {
		void TestBasicOperations() {
			std::println("=== Testing Basic Operations ===");

			LockFreeHashTable<int, std::string> table;

			{
				auto result1 = table.emplace(1, "one");
				assert(result1.has_value());
				assert(result1.value().second);
			}

			{
				auto result2 = table.emplace(2, "two");
				assert(result2.has_value());
				assert(result2.value().second);
			}

			{
				auto result3 = table.emplace(1, "another one");
				assert(result3.has_value());
				assert(!result3.value().second);
			}

			{
				auto ref1 = table.find(1);
				assert(ref1);
				assert(ref1.Get() == "one");
			}

			{
				auto ref2 = table.find(2);
				assert(ref2);
				assert(ref2.Get() == "two");
			}

			{
				auto ref3 = table.find(3);
				assert(!ref3);
			}

			{
				bool erased = table.erase(1);
				assert(erased);
			}

			{
				auto ref4 = table.find(1);
				assert(!ref4);
			}

			{
				bool erased2 = table.erase(1);
				assert(!erased2);
			}

			std::println("Basic operations test passed!");
		}

		void TestConcurrentAccess() {
			std::println("=== Testing Concurrent Access ===");

			LockFreeHashTable<int, int> table;
			constexpr int kThreads = 4;
			constexpr int kIterations = 1000;

			std::vector<std::jthread> threads;
			std::atomic<int> successful_inserts{ 0 };
			std::atomic<int> successful_finds{ 0 };

			for (int t = 0; t < kThreads; ++t) {
				threads.emplace_back([&, thread_id = t] {
					for (int i = 0; i < kIterations; ++i) {
						int key = i * kThreads + thread_id;
						int value = key * 10;

						{
							auto result = table.emplace(key, value);
							if (result.has_value() && result.value().second) {
								successful_inserts.fetch_add(1, std::memory_order_relaxed);
							}
						}

						{
							auto ref = table.find(key);
							if (ref) {
								successful_finds.fetch_add(1, std::memory_order_relaxed);
								assert(ref.Get() == value);
							}
						}

						if (i % 10 == 0) {
							table.erase(key);
						}
					}
					});
			}

			threads.clear();

			std::println("Successful inserts: {}", successful_inserts.load());
			std::println("Successful finds: {}", successful_finds.load());
			std::println("Table size: {}", table.size());

			for (int i = 0; i < kIterations * kThreads; ++i) {
				auto ref = table.find(i);
			}

			std::println("Concurrent access test passed!");
		}

		void TestRehashing() {
			std::println("=== Testing Rehashing ===");

			LockFreeHashTable<int, std::string> table;
			table.max_load_factor(0.1f);

			for (int i = 0; i < 100; ++i) {
				table.emplace(i, std::format("value_{}", i));
			}

			for (int i = 0; i < 100; ++i) {
				auto ref = table.find(i);
				assert(ref);
				assert(ref.Get() == std::format("value_{}", i));
			}

			std::println("Final table size: {}", table.size());
			std::println("Rehashing test passed!");
		}

		void TestMemorySafety() {
			std::println("=== Testing Memory Safety ===");

			{
				LockFreeHashTable<int, std::unique_ptr<int>> table;

				table.emplace(1, std::make_unique<int>(42));

				{
					auto ref1 = table.find(1);
					assert(ref1);
				}

				{
					bool erased = table.erase(1);
					assert(erased);
				}

				{
					assert(ref1);
					assert(*ref1.Get() == 42);
				}

			}

			std::println("Memory safety test passed!");
		}

		void TestExceptionSafety() {
			std::println("=== Testing Exception Safety ===");

			struct ThrowingType {
				int value;
				explicit ThrowingType(int v) : value(v) {
					if (v == 42) {
						throw std::runtime_error("Test exception");
					}
				}
			};

			LockFreeHashTable<int, ThrowingType> table;

			try {
				table.emplace(1, ThrowingType(10));
				assert(table.find(1));
			}
			catch (...) {
				assert(false);
			}

			try {
				table.emplace(42, ThrowingType(42));
				assert(false);
			}
			catch (const std::exception& e) {
				assert(std::string(e.what()) == "Test exception");
			}

			assert(table.find(1));
			assert(!table.find(42));

			std::println("Exception safety test passed!");
		}

		void TestEdgeCases() {
			std::println("=== Testing Edge Cases ===");

			{
				LockFreeHashTable<int, std::string> empty_table;
				assert(empty_table.size() == 0);
				assert(!empty_table.find(1));
				assert(!empty_table.erase(1));
			}

			{
				LockFreeHashTable<int, int> table;
				constexpr int N = 10000;

				for (int i = 0; i < N; ++i) {
					table.emplace(i, i * 10);
				}
				assert(table.size() == N);

				for (int i = 0; i < N; ++i) {
					assert(table.erase(i));
				}
				assert(table.size() == 0);

				for (int i = 0; i < N; ++i) {
					table.emplace(i, i * 20);
				}
				assert(table.size() == N);
			}

			{
				LockFreeHashTable<std::string, int> table;

				for (int i = 0; i < 1000; ++i) {
					table.emplace("key_" + std::to_string(i), i);
				}

				for (int i = 0; i < 1000; ++i) {
					auto ref = table.find("key_" + std::to_string(i));
					assert(ref);
					assert(ref.Get() == i);
				}
			}

			{
				LockFreeHashTable<int, int> table;
				table.emplace(1, 100);
				assert(table.erase(1));
				assert(!table.erase(1));
				assert(table.size() == 0);
			}


			{
				LockFreeHashTable<int, int> table;

				table.max_load_factor(0.01f);

				for (int i = 0; i < 10; ++i) {
					table.emplace(i, i);
				}

				for (int i = 0; i < 10; ++i) {
					assert(table.find(i));
				}
			}

			std::println("Edge cases test passed!");
		}

		void TestConcurrentRehash() {
			std::println("=== Testing Concurrent Rehash ===");

			LockFreeHashTable<int, std::string> table;
			table.max_load_factor(0.1f);

			std::atomic<bool> stop{ false };
			std::atomic<int> total_inserts{ 0 };
			std::atomic<int> total_finds{ 0 };

			std::jthread producer([&] {
				for (int i = 0; i < 10000 && !stop; ++i) {
					if (auto result = table.emplace(i, std::format("value_{}", i));
						result.has_value() && result.value().second) {
						total_inserts.fetch_add(1, std::memory_order_relaxed);
					}
					std::this_thread::sleep_for(std::chrono::microseconds(10));
				}
				});

			std::jthread consumer([&] {
				for (int i = 0; i < 20000 && !stop; ++i) {
					int key = i % 10000;
					if (auto ref = table.find(key)) {
						total_finds.fetch_add(1, std::memory_order_relaxed);
					}
					std::this_thread::sleep_for(std::chrono::microseconds(5));
				}
				});

			std::this_thread::sleep_for(std::chrono::seconds(2));
			stop.store(true);

			std::println("Concurrent inserts during rehash: {}", total_inserts.load());
			std::println("Concurrent finds during rehash: {}", total_finds.load());
			std::println("Final table size: {}", table.size());

			for (int i = 0; i < 100; ++i) {
				table.emplace(i * 100, "test");
			}

			std::println("Concurrent rehash test passed!");
		}

		struct TrackingAllocator {
			static inline std::atomic<size_t> total_allocated;
			static inline std::atomic<size_t> total_deallocated;

			template<typename T>
			struct Allocator {
				using value_type = T;

				Allocator() = default;

				template<typename U>
				Allocator(const Allocator<U>&) noexcept {}

				T* allocate(size_t n) {
					size_t bytes = n * sizeof(T);
					total_allocated.fetch_add(bytes, std::memory_order_relaxed);
					return static_cast<T*>(::operator new(bytes));
				}

				void deallocate(T* p, size_t n) {
					size_t bytes = n * sizeof(T);
					total_deallocated.fetch_add(bytes, std::memory_order_relaxed);
					::operator delete(p);
				}
			};
		};

		void TestMemoryLeak() {
			std::println("=== Testing Memory Leak ===");

			TrackingAllocator::total_allocated = 0;
			TrackingAllocator::total_deallocated = 0;

			{
				using ValueType = std::pair<ImmutableKey<int>, std::string>;
				LockFreeHashTable<
					int,
					std::string,
					std::hash<int>,
					std::equal_to<int>,
					TrackingAllocator::Allocator<ValueType>
				> table;

				for (int i = 0; i < 1000; ++i) {
					table.emplace(i, std::format("value_{}", i));
				}

				for (int i = 0; i < 500; ++i) {
					table.erase(i * 2);
				}

				table.max_load_factor(0.1f);
				for (int i = 1000; i < 2000; ++i) {
					table.emplace(i, std::format("value_{}", i));
				}

				table.clear();
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			size_t allocated = TrackingAllocator::total_allocated.load();
			size_t deallocated = TrackingAllocator::total_deallocated.load();

			std::println("Total allocated: {} bytes", allocated);
			std::println("Total deallocated: {} bytes", deallocated);

			if (allocated == deallocated) {
				std::println("No memory leak detected!");
			}
			else {
				std::println("Potential memory leak! Difference: {} bytes", allocated - deallocated);
				assert(false);
			}
		}

		export void RunLockFreeTableTests() {
			std::println("Starting LockFreeHashTable tests...\n");

			try {
				TestBasicOperations();
				std::println("");

				TestConcurrentAccess();
				std::println("");

				TestRehashing();
				std::println("");

				TestMemorySafety();
				std::println("");

				TestExceptionSafety();
				std::println("");

				TestEdgeCases();
				std::println("");

				TestConcurrentRehash();
				std::println("");

				TestMemoryLeak();
				std::println("");

				std::println("All tests passed!");
			}
			catch (const std::exception& e) {
				std::println("Test failed with exception: {}", e.what());
				std::terminate();
			}
			catch (...) {
				std::println("Test failed with unknown exception");
				std::terminate();
			}
		}

		template<typename Key, typename T, typename Hash = std::hash<Key>,
			typename KeyEqual = std::equal_to<Key>,
			typename Allocator = std::allocator<std::pair<const Key, T>>>
		class LockedUnorderedMap {
		private:
			std::unordered_map<Key, T, Hash, KeyEqual, Allocator> map_;
			mutable std::shared_mutex mutex_;

		public:
			using key_type = Key;
			using mapped_type = T;
			using value_type = std::pair<const Key, T>;

			bool insert(const Key& key, const T& value) {
				std::unique_lock lock(mutex_);
				auto [it, inserted] = map_.emplace(key, value);
				return inserted;
			}

			bool insert(Key&& key, T&& value) {
				std::unique_lock lock(mutex_);
				auto [it, inserted] = map_.emplace(std::move(key), std::move(value));
				return inserted;
			}

			bool insert(value_type const& value) {
				std::unique_lock lock(mutex_);
				auto [it, inserted] = map_.emplace(value);
				return inserted;
			}

			bool insert(value_type&& value) {
				std::unique_lock lock(mutex_);
				auto [it, inserted] = map_.emplace(std::move(value));
				return inserted;
			}

			template<typename... Args>
			bool emplace(Args&&... args) {
				std::unique_lock lock(mutex_);
				auto [it, inserted] = map_.emplace(std::forward<Args>(args)...);
				return inserted;
			}

			std::optional<T> find(const Key& key) const {
				std::shared_lock lock(mutex_);
				auto it = map_.find(key);
				if (it != map_.end()) {
					return it->second;
				}
				return std::nullopt;
			}

			bool erase(const Key& key) {
				std::unique_lock lock(mutex_);
				return map_.erase(key) > 0;
			}

			size_t size() const {
				std::shared_lock lock(mutex_);
				return map_.size();
			}

			void clear() {
				std::unique_lock lock(mutex_);
				map_.clear();
			}
		};

		class BenchmarkTimer {
		private:
			std::chrono::high_resolution_clock::time_point start_;
			std::string name_;

		public:
			explicit BenchmarkTimer(std::string name) : name_(std::move(name)) {
				start_ = std::chrono::high_resolution_clock::now();
			}

			~BenchmarkTimer() {
				auto end = std::chrono::high_resolution_clock::now();
				auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
				std::println("{}: {} μs ({:.3f} ms)", name_, duration.count(), duration.count() / 1000.0);
			}

			auto elapsed() const {
				auto end = std::chrono::high_resolution_clock::now();
				return std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
			}
		};

		struct BenchmarkConfig {
			size_t initial_size = 10000;
			size_t operations_per_thread = 100000;
			size_t thread_count = 4;
			size_t key_range = 1000000;
			bool enable_rehash = true;
			float read_write_ratio = 0.7f;
		};

		template<typename MapType>
		void BenchmarkPureInsert(MapType& map, const BenchmarkConfig& config) {
			std::println("=== Pure Insert Benchmark ({} threads) ===", config.thread_count);

			std::atomic<size_t> successful_inserts{ 0 };
			std::vector<std::jthread> threads;

			auto start = std::chrono::high_resolution_clock::now();

			for (size_t t = 0; t < config.thread_count; ++t) {
				threads.emplace_back([&, thread_id = t] {
					std::mt19937_64 rng(thread_id + 1);
					std::uniform_int_distribution<size_t> dist(0, config.key_range);

					for (size_t i = 0; i < config.operations_per_thread; ++i) {
						size_t key = dist(rng) + (thread_id * config.key_range);
						if (map.insert({ key, key * 10 })) {
							successful_inserts.fetch_add(1, std::memory_order_relaxed);
						}
					}
					});
			}

			threads.clear();
			auto end = std::chrono::high_resolution_clock::now();

			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
			double ops_per_sec = (successful_inserts.load() * 1e6) / duration.count();

			std::println("Successful inserts: {}", successful_inserts.load());
			std::println("Time: {} μs ({:.3f} ms)", duration.count(), duration.count() / 1000.0);
			std::println("Throughput: {:.0f} ops/sec", ops_per_sec);
			std::println("Final size: {}", map.size());
			std::println("");
		}

		template<typename MapType>
		void BenchmarkPureLookup(MapType& map, const BenchmarkConfig& config) {
			std::println("=== Pure Lookup Benchmark ({} threads) ===", config.thread_count);

			std::mt19937_64 rng(42);
			std::uniform_int_distribution<size_t> dist(0, config.key_range);

			for (size_t i = 0; i < config.initial_size; ++i) {
				size_t key = dist(rng);
				map.insert({ key, key * 10 });
			}

			std::atomic<size_t> successful_lookups{ 0 };
			std::vector<std::jthread> threads;

			auto start = std::chrono::high_resolution_clock::now();

			for (size_t t = 0; t < config.thread_count; ++t) {
				threads.emplace_back([&, thread_id = t] {
					std::mt19937_64 rng(thread_id + 100);
					std::uniform_int_distribution<size_t> dist(0, config.key_range);

					for (size_t i = 0; i < config.operations_per_thread; ++i) {
						size_t key = dist(rng);
						if (map.find(key)) {
							successful_lookups.fetch_add(1, std::memory_order_relaxed);
						}
					}
					});
			}

			threads.clear();
			auto end = std::chrono::high_resolution_clock::now();

			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
			double ops_per_sec = (config.operations_per_thread * config.thread_count * 1e6) / duration.count();

			std::println("Successful lookups: {}", successful_lookups.load());
			std::println("Time: {} μs ({:.3f} ms)", duration.count(), duration.count() / 1000.0);
			std::println("Throughput: {:.0f} ops/sec", ops_per_sec);
			std::println("Hit rate: {:.1f}%",
				(successful_lookups.load() * 100.0) / (config.operations_per_thread * config.thread_count));
			std::println("");
		}

		template<typename MapType>
		void BenchmarkMixedOperations(MapType& map, const BenchmarkConfig& config) {
			std::println("=== Mixed Operations Benchmark ({} threads) ===", config.thread_count);

			std::mt19937_64 rng(42);
			std::uniform_int_distribution<size_t> key_dist(0, config.key_range);

			for (size_t i = 0; i < config.initial_size; ++i) {
				size_t key = key_dist(rng);
				map.insert({ key, key * 10 });
			}

			std::atomic<size_t> inserts{ 0 };
			std::atomic<size_t> lookups{ 0 };
			std::atomic<size_t> deletes{ 0 };
			std::vector<std::jthread> threads;

			auto start = std::chrono::high_resolution_clock::now();

			for (size_t t = 0; t < config.thread_count; ++t) {
				threads.emplace_back([&, thread_id = t] {
					std::mt19937_64 rng(thread_id + 200);
					std::uniform_int_distribution<size_t> key_dist(0, config.key_range * 2);
					std::uniform_real_distribution<float> op_dist(0.0f, 1.0f);

					for (size_t i = 0; i < config.operations_per_thread; ++i) {
						float op = op_dist(rng);
						size_t key = key_dist(rng) + (thread_id * config.key_range);

						if (op < config.read_write_ratio) {
							if (map.find(key)) {
								lookups.fetch_add(1, std::memory_order_relaxed);
							}
						}
						else if (op < config.read_write_ratio + 0.2f) {
							if (map.insert({ key, key * 10 })) {
								inserts.fetch_add(1, std::memory_order_relaxed);
							}
						}
						else {
							if (map.erase(key)) {
								deletes.fetch_add(1, std::memory_order_relaxed);
							}
						}
					}
					});
			}

			threads.clear();
			auto end = std::chrono::high_resolution_clock::now();

			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
			double ops_per_sec = (config.operations_per_thread * config.thread_count * 1e6) / duration.count();

			std::println("Total operations: {}", config.operations_per_thread * config.thread_count);
			std::println("Inserts: {}, Lookups: {}, Deletes: {}",
				inserts.load(), lookups.load(), deletes.load());
			std::println("Time: {} μs ({:.3f} ms)", duration.count(), duration.count() / 1000.0);
			std::println("Throughput: {:.0f} ops/sec", ops_per_sec);
			std::println("Final size: {}", map.size());
			std::println("");
		}

		template<typename MapType>
		void BenchmarkScalability() {
			std::println("=== Scalability Benchmark ===");

			constexpr size_t operations_per_thread = 50000;
			constexpr size_t key_range = 1000000;

			std::vector<size_t> thread_counts = { 1, 2, 4, 8, 16 };

			for (size_t threads : thread_counts) {
				MapType map;
				std::atomic<size_t> successful_ops{ 0 };
				std::vector<std::jthread> thread_pool;

				auto start = std::chrono::high_resolution_clock::now();

				for (size_t t = 0; t < threads; ++t) {
					thread_pool.emplace_back([&, thread_id = t] {
						std::mt19937_64 rng(thread_id + 300);
						std::uniform_int_distribution<size_t> dist(0, key_range);

						for (size_t i = 0; i < operations_per_thread; ++i) {
							size_t key = dist(rng) + (thread_id * key_range);
							if (map.insert({ key, key * 10 })) {
								successful_ops.fetch_add(1, std::memory_order_relaxed);
							}
						}
						});
				}

				thread_pool.clear();
				auto end = std::chrono::high_resolution_clock::now();

				auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
				double ops_per_sec = (successful_ops.load() * 1e6) / duration.count();

				std::println("Threads: {:2d} | Time: {:8} μs | Throughput: {:8.0f} ops/sec | Speedup: {:.2f}x",
					threads, duration.count(), ops_per_sec,
					threads == 1 ? 1.0 : ops_per_sec / (successful_ops.load() / threads / (duration.count() / 1e6)));
			}

			std::println("");
		}

		void BenchmarkRehashPerformance() {
			std::println("=== Rehash Performance Benchmark ===");

			constexpr size_t initial_capacity = 1000;
			constexpr size_t final_size = 100000;

			{
				std::println("Testing LockFreeHashTable:");
				LockFreeHashTable<size_t, size_t> lf_table;
				lf_table.max_load_factor(0.5f);

				auto start = std::chrono::high_resolution_clock::now();

				for (size_t i = 0; i < final_size; ++i) {
					lf_table.emplace(i, i * 10);

					if (i % 1000 == 0) {
						lf_table.find(i / 2);
					}
				}

				auto end = std::chrono::high_resolution_clock::now();
				auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

				std::println("  Time to insert {} elements: {} μs ({:.3f} ms)",
					final_size, duration.count(), duration.count() / 1000.0);
				std::println("  Final size: {}", lf_table.size());
			}

			{
				std::println("\nTesting LockedUnorderedMap:");
				LockedUnorderedMap<size_t, size_t> locked_map;

				auto start = std::chrono::high_resolution_clock::now();

				for (size_t i = 0; i < final_size; ++i) {
					locked_map.insert(i, i * 10);

					if (i % 1000 == 0) {
						locked_map.find(i / 2);
					}
				}

				auto end = std::chrono::high_resolution_clock::now();
				auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

				std::println("  Time to insert {} elements: {} μs ({:.3f} ms)",
					final_size, duration.count(), duration.count() / 1000.0);
				std::println("  Final size: {}", locked_map.size());
			}

			std::println("");
		}

		export void RunCompleteBenchmarkSuite() {
			std::println("==========================================");
			std::println("  LockFreeHashTable vs LockedUnorderedMap  ");
			std::println("          Benchmark Suite                 ");
			std::println("==========================================");
			std::println("");

			BenchmarkConfig config;
			config.initial_size = 10000;
			config.operations_per_thread = 100000;
			config.thread_count = 4;
			config.key_range = 1000000;
			config.read_write_ratio = 0.7f;

			std::println("TEST 1: PURE INSERT PERFORMANCE");
			{
				std::println("\n--- LockFreeHashTable ---");
				LockFreeHashTable<size_t, size_t> lf_table;
				BenchmarkPureInsert(lf_table, config);

				std::println("\n--- LockedUnorderedMap ---");
				LockedUnorderedMap<size_t, size_t> locked_map;
				BenchmarkPureInsert(locked_map, config);
			}

			std::println("\nTEST 2: PURE LOOKUP PERFORMANCE");
			{
				std::println("\n--- LockFreeHashTable ---");
				LockFreeHashTable<size_t, size_t> lf_table;
				BenchmarkPureLookup(lf_table, config);

				std::println("\n--- LockedUnorderedMap ---");
				LockedUnorderedMap<size_t, size_t> locked_map;
				BenchmarkPureLookup(locked_map, config);
			}

			std::println("\nTEST 3: MIXED OPERATIONS PERFORMANCE");
			{
				std::println("\n--- LockFreeHashTable ---");
				LockFreeHashTable<size_t, size_t> lf_table;
				BenchmarkMixedOperations(lf_table, config);

				std::println("\n--- LockedUnorderedMap ---");
				LockedUnorderedMap<size_t, size_t> locked_map;
				BenchmarkMixedOperations(locked_map, config);
			}

			std::println("\nTEST 4: SCALABILITY (PURE INSERT)");
			{
				std::println("\n--- LockFreeHashTable ---");
				BenchmarkScalability<LockFreeHashTable<size_t, size_t>>();

				std::println("\n--- LockedUnorderedMap ---");
				BenchmarkScalability<LockedUnorderedMap<size_t, size_t>>();
			}

			std::println("\nTEST 5: REHASH PERFORMANCE");
			BenchmarkRehashPerformance();

			std::println("\nTEST 6: HIGH CONTENTION SCENARIO");
			{
				constexpr size_t high_contention_threads = 16;
				constexpr size_t small_key_range = 100;

				BenchmarkConfig high_contention_config = config;
				high_contention_config.thread_count = high_contention_threads;
				high_contention_config.key_range = small_key_range;

				std::println("\n--- LockFreeHashTable (High Contention) ---");
				LockFreeHashTable<size_t, size_t> lf_table_hc;
				BenchmarkPureInsert(lf_table_hc, high_contention_config);

				std::println("\n--- LockedUnorderedMap (High Contention) ---");
				LockedUnorderedMap<size_t, size_t> locked_map_hc;
				BenchmarkPureInsert(locked_map_hc, high_contention_config);
			}

			std::println("==========================================");
			std::println("       Benchmark Suite Complete          ");
			std::println("==========================================");
		}

	}

}